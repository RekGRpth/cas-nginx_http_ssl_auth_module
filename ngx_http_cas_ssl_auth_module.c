#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>


#define CAS_SSL_AUTH_ERROR "CAS-PKI-AUTH-ERROR"


static void *ngx_http_cas_ssl_auth_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_cas_ssl_auth_merge_loc_conf(ngx_conf_t *cf, void *parent, void*child);
static ngx_int_t ngx_http_cas_ssl_auth_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_cas_ssl_auth_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_cas_ssl_auth_done(ngx_http_request_t *r, void *data, ngx_int_t rc);

typedef struct {
    ngx_str_t           error;
    ngx_uint_t          done;  
    ngx_uint_t          status;
    ngx_http_request_t *subrequest; 
} ngx_http_cas_ssl_auth_ctx_t;


typedef struct {
    ngx_flag_t auth;
    ngx_str_t uri; 
} ngx_http_cas_ssl_auth_conf_t;


static ngx_command_t ngx_http_cas_ssl_auth_commands[] = {
    {
        ngx_string("cas_ssl_auth_uri"), 
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_cas_ssl_auth_conf_t, uri),
        NULL,
    },
    {
        ngx_string("cas_ssl_auth"), 
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_cas_ssl_auth_conf_t, auth),
        NULL,
    },
    ngx_null_command
};


static ngx_http_module_t ngx_http_cas_ssl_auth_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_cas_ssl_auth_init,                   /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_cas_ssl_auth_create_loc_conf,         /* create location configuration */
    ngx_http_cas_ssl_auth_merge_loc_conf,          /* merge location configuration */
};


ngx_module_t  ngx_http_cas_ssl_auth_module = {
    NGX_MODULE_V1,
    &ngx_http_cas_ssl_auth_module_ctx,   /* module context */
    ngx_http_cas_ssl_auth_commands,      /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_cas_ssl_auth_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_cas_ssl_auth_conf_t *hcsacf;    
    
    hcsacf = ngx_pcalloc(cf->pool, sizeof(ngx_http_cas_ssl_auth_conf_t)) ;
    if (hcsacf == NULL) {
        return NULL;
    }
    
    hcsacf->uri.data = NULL;
    hcsacf->uri.len = NGX_CONF_UNSET_UINT;
    hcsacf->auth = NGX_CONF_UNSET;
    
    return hcsacf;
}


static char *
ngx_http_cas_ssl_auth_merge_loc_conf(ngx_conf_t *cf, void *parent, void*child)
{
    ngx_http_cas_ssl_auth_conf_t *prev = parent;
    ngx_http_cas_ssl_auth_conf_t *conf = child;
    
    ngx_conf_merge_str_value(conf->uri, prev->uri, "");
    ngx_conf_merge_value(conf->auth, prev->auth, 1);
    
    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_cas_ssl_auth_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;
    
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    
    *h = ngx_http_cas_ssl_auth_handler;
    return NGX_OK;
}


static ngx_int_t
ngx_http_cas_ssl_auth_handler(ngx_http_request_t *r) 
{
    ngx_buf_t                    *b;
    ngx_chain_t                   out;
    ngx_http_request_t           *sr;
    ngx_http_cas_ssl_auth_ctx_t  *ctx;
    ngx_http_post_subrequest_t   *ps;
    ngx_http_cas_ssl_auth_conf_t *hcsacf;
    
    hcsacf = ngx_http_get_module_loc_conf(r, ngx_http_cas_ssl_auth_module);
    if (hcsacf->uri.len == 0) {
        return NGX_DECLINED;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_cas_ssl_auth_module);
    if (ctx != NULL) {
        if (!ctx->done) {
            return NGX_AGAIN;  
        }
        if (ctx->status == NGX_HTTP_OK) {
            return NGX_OK;
        }
        r->headers_out.status = NGX_HTTP_FORBIDDEN;
        r->headers_out.content_length_n = ctx->error.len;
        ngx_str_set(&r->headers_out.content_type, "text/html");
        
        b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        out.buf = b;
        out.next = NULL;
        b->pos = ctx->error.data;
        b->last = ctx->error.data + ctx->error.len;
        b->memory = 1;
        b->last_buf = 1;
        
        return NGX_HTTP_FORBIDDEN;
    }
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_cas_ssl_auth_ctx_t));
    if (ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    ps = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (ps == NULL) {
        return NGX_ERROR;
    }

    ps->handler = ngx_http_cas_ssl_auth_done;
    ps->data = ctx;
    
    if (ngx_http_subrequest(r, &hcsacf->uri, NULL, &sr, ps,
                            NGX_HTTP_SUBREQUEST_WAITED)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    ctx->subrequest = sr;
    sr->header_only = 1;
    ngx_http_set_ctx(r, ctx, ngx_http_cas_ssl_auth_module);

    return NGX_AGAIN;
}


static ngx_int_t
ngx_http_cas_ssl_auth_done(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_list_part_t               *part = &r->headers_out.headers.part;
    ngx_table_elt_t               *header = part->elts;
    ngx_http_cas_ssl_auth_ctx_t   *ctx = data;
    
    ngx_uint_t i;

    ctx->done = 1;
    ctx->status = r->headers_out.status;

    for (i = 0; /* void */ ; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }
        if (header[i].hash == 0) {
            continue;
        }

        if (0 == ngx_strncasecmp(header[i].key.data,
                (u_char *) CAS_SSL_AUTH_ERROR,
                header[i].key.len
            ))
        {
            ctx->error.data = header[i].value.data;
            ctx->error.len = header[i].value.len;
            return rc;
        }
    }
    
    return rc;
}