#define BOOST_TEST_MODULE uri_parse_test
#include <boost/test/included/unit_test.hpp>
#include <servlet/uri.h>

using namespace servlet;

//static bool check_integrity(const URI& uri)
//{
//    string_view view = uri.uri_view();
//    auto b = view.begin();
//    auto e = view.end();
//    return uri.scheme().begin() >= b && uri.scheme().end() <= e &&
//           uri.scheme().begin() >= b && uri.scheme().end() <= e &&
//           uri.user_info().begin() >= b && uri.user_info().end() <= e &&
//           uri.host().begin() >= b && uri.host().end() <= e &&
//           uri.port_view().begin() >= b && uri.port_view().end() <= e &&
//           uri.path().begin() >= b && uri.path().end() <= e &&
//           uri.query().begin() >= b && uri.query().end() <= e &&
//           uri.fragment().begin() >= b && uri.fragment().end() <= e;
//}
//
//static bool check_uri(const URI& uri, string_view uri_view, string_view scheme, string_view user_info, string_view host,
//                      string_view port_view, uint16_t port, string_view path, string_view query, string_view fragment)
//{
//    return (uri_view.empty() ? uri.uri_view().empty() : uri_view == uri.uri_view()) &&
//           (scheme.empty() ? uri.scheme().empty() : scheme == uri.scheme()) &&
//           (user_info.empty() ? uri.user_info().empty() : user_info == uri.user_info()) &&
//           (host.empty() ? uri.host().empty() : host == uri.host()) &&
//           (port_view.empty() ? uri.port_view().empty() : port_view == uri.port_view()) &&
//           (path.empty() ? uri.path().empty() : path == uri.path()) &&
//           (query.empty() ? uri.query().empty() : query == uri.query()) &&
//           (fragment.empty() ? uri.fragment().empty() : fragment == uri.fragment()) &&
//           (port == uri.port()) &&
//           check_integrity(uri);
//}
//
//BOOST_AUTO_TEST_CASE(mail_uri_parse_test)
//{
//    URI uri{"mailto:mduerst@ifi.unizh.ch"};
//    BOOST_CHECK(check_uri(uri, "mailto:mduerst@ifi.unizh.ch", "mailto", "mduerst", "ifi.unizh.ch", "", 0, "", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_user_info)
//{
//    URI uri{"http://user@www.example.com:80/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://user@www.example.com:80/path?query#fragment", "http", "user",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_unset_user_info_and_host)
//{
//    BOOST_CHECK_THROW(URI{"http://:80/path?query#fragment"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_empty_user_info)
//{
//    BOOST_CHECK_THROW(URI{"http://@www.example.com:80/path?query#fragment"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_user_info_unset_host)
//{
//    BOOST_CHECK_THROW(URI{"http://user@:80/path?query#fragment"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_unset_user_info)
//{
//    URI uri{"http://www.example.com:80/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
//                          "www.example.com", "80", 80, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_empty_port_empty_path)
//{
//    URI uri{"http://www.example.com"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com", "http", "", "www.example.com", "", 0, "", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_valid_port_empty_path)
//{
//    URI uri{"http://www.example.com:80"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80", "http", "", "www.example.com", "80", 80, "", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_port_path)
//{
//    URI uri{"http://www.example.com:80/path"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com:80/path", "http", "", "www.example.com",
//                          "80", 80, "/path", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_path)
//{
//    URI uri{"http://www.example.com/path"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path", "http", "", "www.example.com", "", 0, "/path", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_path_and_query)
//{
//    URI uri{"http://www.example.com/path?query"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path?query", "http",
//                          "", "www.example.com", "", 0, "/path", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_path_query_and_fragment)
//{
//    URI uri{"http://www.example.com/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path?query#fragment", "http",
//                          "", "www.example.com", "", 0, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_hierarchical_part_valid_host_path_and_fragment)
//{
//    URI uri{"http://www.example.com/path#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path#fragment", "http",
//                          "", "www.example.com", "", 0, "/path", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_fragment)
//{
//    BOOST_CHECK_THROW(URI{"http://www.example.com/path#%fragment"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_fragment_with_pct_encoded_char)
//{
//    URI uri{"http://www.example.com/path#%ffragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path#%ffragment", "http",
//                          "", "www.example.com", "", 0, "/path", "", "%ffragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_fragment_with_unreserved_char)
//{
//    URI uri{"http://www.example.com/path#fragment-"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path#fragment-", "http",
//                          "", "www.example.com", "", 0, "/path", "", "fragment-"));
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_fragment_with_gen_delim)
//{
//    BOOST_CHECK_THROW(URI{"http://www.example.com/path#frag#ment"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_fragment_with_forward_slash_and_question_mark)
//{
//    URI uri{"http://www.example.com/path#frag/ment?"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path#frag/ment?", "http",
//                          "", "www.example.com", "", 0, "/path", "", "frag/ment?"));
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_query)
//{
//    BOOST_CHECK_THROW(URI{"http://www.example.com/path?%query"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_query_with_pct_encoded_char)
//{
//    URI uri{"http://www.example.com/path?%00query"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path?%00query", "http",
//                          "", "www.example.com", "", 0, "/path", "%00query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_query_with_unreserved_char)
//{
//    URI uri{"http://www.example.com/path?query-"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path?query-", "http",
//                          "", "www.example.com", "", 0, "/path", "query-", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_valid_query_with_sub_delim)
//{
//    URI uri{"http://www.example.com/path?qu$ery"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com/path?qu$ery", "http",
//                          "", "www.example.com", "", 0, "/path", "qu$ery", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_empty_port_with_path)
//{
//    URI uri{"http://123.34.23.56:/"};
//    BOOST_CHECK(check_uri(uri, "http://123.34.23.56:/", "http",
//                          "", "123.34.23.56", "", 0, "/", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_empty_port)
//{
//    URI uri{"http://123.34.23.56:"};
//    BOOST_CHECK(check_uri(uri, "http://123.34.23.56:", "http",
//                          "", "123.34.23.56", "", 0, "", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_port_with_path)
//{
//    BOOST_CHECK_THROW(URI{"http://123.34.23.56:6662626/"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_ipv6_address)
//{
//    URI uri{"http://[1080:0:0:0:8:800:200C:417A]"};
//    BOOST_CHECK(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]", "http",
//                          "", "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_ipv6_address_with_path)
//{
//    URI uri{"http://[1080:0:0:0:8:800:200C:417A]/"};
//    BOOST_CHECK(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]/", "http",
//                          "", "[1080:0:0:0:8:800:200C:417A]", "", 0, "/", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_ipv6_address)
//{
//    BOOST_CHECK_THROW(URI{"http://[1080:0:0:0:8:800:200C:417A"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_invalid_ipv6_address_with_path)
//{
//    BOOST_CHECK_THROW(URI{"http://[1080:0:0:0:8:800:200C:417A/"}, uri_syntax_error);
//}
//
//BOOST_AUTO_TEST_CASE(test_opaque_uri_with_one_slash)
//{
//    URI uri{"scheme:/path/"};
//    BOOST_CHECK(check_uri(uri, "scheme:/path/", "scheme", "", "", "", 0, "/path/", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_query_with_empty_path)
//{
//    URI uri{"http://www.example.com?query"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com?query", "http", "", "www.example.com", "", 0, "", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_query_with_user_info_and_empty_path)
//{
//    URI uri{"http://user@www.example.com?query"};
//    BOOST_CHECK(check_uri(uri, "http://user@www.example.com?query", "http", "user",
//                          "www.example.com", "", 0, "", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_fragment_with_empty_path)
//{
//    URI uri{"http://www.example.com#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://www.example.com#fragment", "http", "",
//                          "www.example.com", "", 0, "", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_fragment_with_user_info_and_empty_path)
//{
//    URI uri{"http://user@www.example.com#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://user@www.example.com#fragment", "http", "user",
//                          "www.example.com", "", 0, "", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_query_with_empty_path_and_ipv6_address)
//{
//    URI uri{"http://[1080:0:0:0:8:800:200C:417A]?query"};
//    BOOST_CHECK(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]?query", "http", "",
//                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_query_with_user_info_empty_path_and_ipv6_address)
//{
//    URI uri{"http://user@[1080:0:0:0:8:800:200C:417A]?query"};
//    BOOST_CHECK(check_uri(uri, "http://user@[1080:0:0:0:8:800:200C:417A]?query", "http", "user",
//                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_fragment_with_empty_path_and_ipv6_address)
//{
//    URI uri{"http://[1080:0:0:0:8:800:200C:417A]#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]#fragment", "http", "",
//                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_fragment_with_user_info_empty_path_and_ipv6_address)
//{
//    URI uri{"http://user@[1080:0:0:0:8:800:200C:417A]#fragment"};
//    BOOST_CHECK(check_uri(uri, "http://user@[1080:0:0:0:8:800:200C:417A]#fragment", "http", "user",
//                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_pct_encoded_user_info)
//{
//    URI uri{"http://user%3f@www.example.com/"};
//    BOOST_CHECK(check_uri(uri, "http://user%3f@www.example.com/", "http", "user%3f",
//                          "www.example.com", "", 0, "/", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_path_with_query_and_fragment)
//{
//    URI uri{"/path?query#fragment"};
//    BOOST_CHECK(check_uri(uri, "/path?query#fragment", "", "", "", "", 0, "/path", "query", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_path_with_query)
//{
//    URI uri{"/path?query"};
//    BOOST_CHECK(check_uri(uri, "/path?query", "", "", "", "", 0, "/path", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_path_with_fragment)
//{
//    URI uri{"/path#fragment"};
//    BOOST_CHECK(check_uri(uri, "/path#fragment", "", "", "", "", 0, "/path", "", "fragment"));
//}
//
//BOOST_AUTO_TEST_CASE(test_path_only)
//{
//    URI uri{"/path"};
//    BOOST_CHECK(check_uri(uri, "/path", "", "", "", "", 0, "/path", "", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_query_only)
//{
//    URI uri{"?query"};
//    BOOST_CHECK(check_uri(uri, "?query", "", "", "", "", 0, "", "query", ""));
//}
//
//BOOST_AUTO_TEST_CASE(test_fragment_only)
//{
//    URI uri{"#fragment"};
//    BOOST_CHECK(check_uri(uri, "#fragment", "", "", "", "", 0, "", "", "fragment"));
//}
