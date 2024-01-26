#include <stdio.h>
// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "utils/httplib.h"

int main()
{
    printf("Hello World\n");

    // HTTP
    httplib::Server svr;

    // HTTPS
    // httplib::SSLServer svr;

    svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
      res.set_content("Hello World!", "text/plain");
    });

    svr.listen("0.0.0.0", 8080);
    return 0;
}