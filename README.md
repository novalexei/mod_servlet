#C++ Servlet API and container

This is where simplicity of Servlet API, power of C++ language and performance of Apache2 httpd server meet!

This is my attempt to implement C++ API for server side web development. The API is based on [Java Servlet API](http://docs.oracle.com/javaee/6/tutorial/doc/bnafd.html). The container is implemented as an Apache2 module.

What is a servlet anyways? A servlet is a server-resident program that typically runs automatically in response to user request. It can process user input and/or generate the output dynamicly.

The features of __mod_servlet__ API:

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

How a simple HTML generating servlet looks like? This is how:

```cpp
#include <iostream>
#include <servlet/servlet.h>

struct hello_servlet : public servlet::http_servlet
{
    void do_get(http_request& req, http_response& resp) override
    {
        resp.set_content_type("text/html");
        resp.get_output_stream() << "<h1>Hello, mod_servlet!</h1>";
    }
};

SERVLET_EXPORT(helloServlet, hello_servlet)
```

Full example with build instructions is [here](https://github.com/novalexei/mod_servlet/wiki/First-mod_servlet-application). You also get with it [exceptional performance](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-and-Java-Servlets-Performance-comparison). Interested?

You'll need to [build and install it](https://github.com/novalexei/mod_servlet/wiki/Build-Instructions) first. Then [configure it with Apache2](https://github.com/novalexei/mod_servlet/wiki/Apache2-server-configuration). And you are ready to start [development of your own web applications](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-programming-guide). Extensive [API reference is also available](https://novalexei.github.io/mod_servlet/html/index.html).

Documentation pages:

- [Programming Guide](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-programming-guide)
- [Apache2 configuration](https://github.com/novalexei/mod_servlet/wiki/Apache2-server-configuration)
- [__mod_servlet__ API reference](https://novalexei.github.io/mod_servlet/html/index.html)
- [Deployment descriptor (_web.xml_) reference](https://github.com/novalexei/mod_servlet/wiki/web.xml-reference)
- [Build Instructions](https://github.com/novalexei/mod_servlet/wiki/Build-Instructions)
- [Quick performance comparison of __mod_servlet__ and Java Servlets](https://github.com/novalexei/mod_servlet/wiki/mod_servlet-and-Java-Servlets-Performance-comparison)

