#define BOOST_TEST_MODULE uri_set_test
#include <boost/test/included/unit_test.hpp>
#include <servlet/uri.h>

using namespace servlet;

//static bool check_uri(const URI& uri, string_view uri_view, string_view scheme, string_view user_info, string_view host,
//                      string_view port_view, uint16_t port, string_view path, string_view query, string_view fragment)
//{
//    if (uri_view.empty() ? !uri.uri_view().empty() : uri_view != uri.uri_view()) return false;
//    if (scheme.empty() ? !uri.scheme().empty() : scheme != uri.scheme()) return false;
//    if (user_info.empty() ? !uri.user_info().empty() : user_info != uri.user_info()) return false;
//    if (host.empty() ? !uri.host().empty() : host != uri.host()) return false;
//    if (port_view.empty() ? !uri.port_view().empty() : port_view != uri.port_view()) return false;
//    if (path.empty() ? !uri.path().empty() : path != uri.path()) return false;
//    if (query.empty() ? !uri.query().empty() : query != uri.query()) return false;
//    if (fragment.empty() ? !uri.fragment().empty() : fragment != uri.fragment()) return false;
//    if (port != uri.port()) return false;
//    return true;
//}
//
//BOOST_AUTO_TEST_CASE(set1_test)
//{
//    URI uri{"http://www.example.com:80/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_user_info("user");
//    BOOST_CHECK(check_uri(uri, "http://user@www.example.com:80/path?query#fragment", "http", "user",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_user_info("");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_scheme("");
//    BOOST_CHECK(check_uri(uri, "www.example.com:80/path?query#fragment", "", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_scheme("https");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_port("");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com/path?query#fragment", "https", "",
//                          "www.example.com", "", 0, "/path", "query", "fragment"));
//    uri.set_port("80");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_path("");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "", "query", "fragment"));
//    uri.set_path("/path");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_query("");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "", "fragment"));
//    uri.set_query("query");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_fragment("");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", ""));
//    uri.set_fragment("fragment");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?query#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_query("");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "", "fragment"));
//    uri.add_to_query("n1", "v1");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?n1=v1#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "n1=v1", "fragment"));
//    uri.add_to_query("n2", "v2");
//    BOOST_CHECK(check_uri(uri, "https://www.example.com:80/path?n1=v1&n2=v2#fragment", "https", "",
//                          "www.example.com", "80", 80, "/path", "n1=v1&n2=v2", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(set2_test)
//{
//    URI uri{"http://www.example.com:80/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//    uri.set_host("[1080:0:0:0:8:800:200C:417A]");
//    BOOST_CHECK(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]:80/path?query#fragment", "http", "",
//                          "[1080:0:0:0:8:800:200C:417A]", "80", 80, "/path", "query", "fragment"));
//    uri.set_host("www.example.com");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(set3_test)
//{
//    URI uri{"http://www.example.com#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com#fragment", "http", "",
//                          "www.example.com", "", 0, "", "", "fragment"));
//    uri.set_port(80);
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80#fragment", "http", "",
//                          "www.example.com", "80", 80, "", "", "fragment"));
//    uri.set_query("query");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "", "query", "fragment"));
//    uri.set_path("/path");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(set4_test)
//{
//    URI uri{"http://www.example.com"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com", "http", "",
//                          "www.example.com", "", 0, "", "", ""));
//    uri.set_query("query");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com?query", "http", "",
//                          "www.example.com", "", 0, "", "query", ""));
//    uri.set_fragment("fragment");
//    BOOST_CHECK(check_uri(uri, "http://www.example.com?query#fragment", "http", "",
//                          "www.example.com", "", 0, "", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(set5_test)
//{
//    URI uri{"/path"};
//    BOOST_CHECK(check_uri(uri, "/path", "", "", "", "", 0, "/path", "", ""));
//    uri.set_query("query");
//    BOOST_CHECK(check_uri(uri, "/path?query", "", "", "", "", 0, "/path", "query", ""));
//    uri.set_fragment("fragment");
//    BOOST_CHECK(check_uri(uri, "/path?query#fragment", "", "", "", "", 0, "/path", "query", "fragment"));
//    uri.set_scheme("http");
//    BOOST_CHECK(check_uri(uri, "http:///path?query#fragment", "http", "", "", "", 0, "/path", "query", "fragment"));
//    uri.set_port("80");
//    BOOST_CHECK(check_uri(uri, "http://:80/path?query#fragment", "http", "",
//                          "", "80", 80, "/path", "query", "fragment"));
//    uri.set_host("localhost");
//    BOOST_CHECK(check_uri(uri, "http://localhost:80/path?query#fragment", "http", "",
//                          "localhost", "80", 80, "/path", "query", "fragment"));
//    uri.set_user_info("user");
//    BOOST_CHECK(check_uri(uri, "http://user@localhost:80/path?query#fragment", "http", "user",
//                          "localhost", "80", 80, "/path", "query", "fragment"));
//}
