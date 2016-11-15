#C++ Servlet API and container

This is where meet simplicity of Servlet API, power of C++ and performance of Apache2 httpd server!

This is my attempt to implement C++ API for server side web development based on Java Servlet API as an Apache module.

The features of Java Servlet API implemented in __mod_servlet__

- Servlets;
- Filters;
- Sessions;
- Cookies;
- Redirects
- Server side forwards and includes;
- SSL related information;
- Request streams, including multipart uploads;
- Serving static content from web application;
- and probably a lot of other stuff I cannot remember now.

How simple HTML generating servlet look like? This is how:

```cpp
#include <iostream>
#include <servlet/servlet.h>

class hello_servlet : public servlet::http_servlet
{
public:
    void do_get(http_request& req, http_response& resp) override
    {
        resp.set_content_type("text/html");
        resp.get_output_stream() << "<h1>Hello, mod_servlet!</h1>";
    }
};

SERVLET_EXPORT(helloServlet, hello_servlet)
```

Full example with build instructions is [here](https://github.com/novalexei/mod_servlet/wiki/First-mod_servlet-application). You also get with it [exceptional performance](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-and-Java-Servlets-Performance-comparison). Interested?

You'll need to [build and install it](https://github.com/novalexei/mod_servlet/wiki/Build-Instructions) first.

And here are usefull links:

- [Programming Guide](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-programming-guide)
- [Apache2 configuration](https://github.com/novalexei/mod_servlet/wiki/Apache2-server-configuration)
- [__mod_servlet__ API reference](https://novalexei.github.io/mod_servlet/html/index.html)
- [Deployment descriptor (_web.xml_) reference](https://github.com/novalexei/mod_servlet/wiki/web.xml-reference)
- [Build Instructions](https://github.com/novalexei/mod_servlet/wiki/Build-Instructions)
- [Quick performance comparison of __mod_servlet__ and Java Servlets](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-and-Java-Servlets-Performance-comparison)

