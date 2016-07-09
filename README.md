[CAS]: https://github.com/cas-x/cas-server
# ngx_http_cas_ssl_auth_module
![Branch master](https://img.shields.io/badge/branch-master-brightgreen.svg?style=flat-square)[![Build](https://api.travis-ci.org/detailyang/cas-nginx_http_ssl_auth_module.svg)](https://travis-ci.org/detailyang/cas-nginx_http_ssl_auth_module)[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/detailyang/cas-nginx_http_ssl_auth_module/master/LICENSE)[![release](https://img.shields.io/github/release/detailyang/cas-nginx_http_ssl_auth_module.svg)](https://github.com/detailyang/cas-nginx_http_ssl_auth_module/releases)

A component for nginx module integrated with [CAS]


Table of Contents
-----------------
* [How-To-Use](#how-to-use)
* [Requirements](#requirements)
* [Direction](#direction)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)


How-To-Use
----------------

ngx_http_cas_ssl_auth_module is the same as ngx_http_auth_module but for [CAS]
For example:

```bash
cas_ssl_auth_uri /auth;
location / {
        cas_request /auth;
        proxy_pass http://127.0.0.1:12345;
}

location = /auth {
        internal;
        proxy_pass https:/cas.example.com/?client_serial=$ssl_client_serial&client_sdn=$ssl_client_s_dn&client_idn=$ssl_client_i_dn;
}
```

Requirements
------------

ngx_http_cas_ssl_auth_module requires the following to run:

 * [nginx](http://nginx.org/) or other forked version like [openresty](http://openresty.org/)、[tengine](http://tengine.taobao.org/)
 * [CAS](https://github.com/detailyang/cas-server)

Direction
------------

* cas_ssl_auth_uri: enable cas authentication        
Syntax:     cas_ssl_auth_uri url       
Default:    -         
Context:    server|location         

```
cas_ssl_auth_uri /auth;
location / {
        cas_request /auth;
        proxy_pass http://127.0.0.1:12345;
}

location = /auth {
        internal;
        proxy_pass https:/cas.example.com/?client_serial=$ssl_client_serial&client_sdn=$ssl_client_s_dn&client_idn=$ssl_client_i_dn;
}
```

Contributing
------------

To contribute to ngx_http_cas_module, clone this repo locally and commit your code on a separate branch.


Author
------

> GitHub [@detailyang](https://github.com/detailyang)


License
-------
ngx_http_cas_ssl_auth_module is licensed under the [MIT] license.

[MIT]: https://github.com/detailyang/ybw/blob/master/licenses/MIT
