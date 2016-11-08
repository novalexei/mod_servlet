#include <gtest/gtest.h>
#include <servlet/uri.h>

using namespace servlet;

static bool check_integrity(const URI& uri)
{
    string_view view = uri.uri_view();
    auto b = view.begin();
    auto e = view.end();
    return uri.scheme().begin() >= b && uri.scheme().end() <= e &&
           uri.scheme().begin() >= b && uri.scheme().end() <= e &&
           uri.user_info().begin() >= b && uri.user_info().end() <= e &&
           uri.host().begin() >= b && uri.host().end() <= e &&
           uri.port_view().begin() >= b && uri.port_view().end() <= e &&
           uri.path().begin() >= b && uri.path().end() <= e &&
           uri.query().begin() >= b && uri.query().end() <= e &&
           uri.fragment().begin() >= b && uri.fragment().end() <= e;
}

static bool check_uri(const URI& uri, string_view uri_view, string_view scheme, string_view user_info, string_view host,
                      string_view port_view, uint16_t port, string_view path, string_view query, string_view fragment)
{
    return (uri_view.empty() ? uri.uri_view().empty() : uri_view == uri.uri_view()) &&
           (scheme.empty() ? uri.scheme().empty() : scheme == uri.scheme()) &&
           (user_info.empty() ? uri.user_info().empty() : user_info == uri.user_info()) &&
           (host.empty() ? uri.host().empty() : host == uri.host()) &&
           (port_view.empty() ? uri.port_view().empty() : port_view == uri.port_view()) &&
           (path.empty() ? uri.path().empty() : path == uri.path()) &&
           (query.empty() ? uri.query().empty() : query == uri.query()) &&
           (fragment.empty() ? uri.fragment().empty() : fragment == uri.fragment()) &&
           (port == uri.port()) &&
           check_integrity(uri);
}

TEST(parse_test, mail_uri_parse_test)
{
    URI uri{"mailto:mduerst@ifi.unizh.ch"};
    ASSERT_TRUE(check_uri(uri, "mailto:mduerst@ifi.unizh.ch", "mailto", "mduerst", "ifi.unizh.ch", "", 0, "", "", ""));
}

TEST(parse_test, test_hierarchical_part_valid_user_info)
{
    URI uri{"http://user@www.example.com:80/path?query#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://user@www.example.com:80/path?query#fragment", "http", "user",
                          "www.example.com", "80", 80, "/path", "query", "fragment"));
}

TEST(parse_test, test_hierarchical_part_unset_user_info_and_host)
{
    ASSERT_THROW(URI{"http://:80/path?query#fragment"}, uri_syntax_error);
}

TEST(parse_test, test_hierarchical_part_empty_user_info)
{
    ASSERT_THROW(URI{"http://@www.example.com:80/path?query#fragment"}, uri_syntax_error);
}

TEST(parse_test, test_hierarchical_part_valid_user_info_unset_host)
{
    ASSERT_THROW(URI{"http://user@:80/path?query#fragment"}, uri_syntax_error);
}

TEST(parse_test, test_hierarchical_part_unset_user_info)
{
    URI uri{"http://www.example.com:80/path?query#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com:80/path?query#fragment", "http", "",
                          "www.example.com", "80", 80, "/path", "query", "fragment"));
}

TEST(parse_test, test_hierarchical_part_valid_host_empty_port_empty_path)
{
    URI uri{"http://www.example.com"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com", "http", "", "www.example.com", "", 0, "", "", ""));
}

TEST(parse_test, test_hierarchical_part_valid_host_valid_port_empty_path)
{
    URI uri{"http://www.example.com:80"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com:80", "http", "", "www.example.com", "80", 80, "", "", ""));
}

TEST(parse_test, test_hierarchical_part_valid_host_port_path)
{
    URI uri{"http://www.example.com:80/path"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com:80/path", "http", "", "www.example.com",
                          "80", 80, "/path", "", ""));
}

TEST(parse_test, test_hierarchical_part_valid_host_path)
{
    URI uri{"http://www.example.com/path"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path", "http", "", "www.example.com", "", 0, "/path", "", ""));
}

TEST(parse_test, test_hierarchical_part_valid_host_path_and_query)
{
    URI uri{"http://www.example.com/path?query"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path?query", "http",
                          "", "www.example.com", "", 0, "/path", "query", ""));
}

TEST(parse_test, test_hierarchical_part_valid_host_path_query_and_fragment)
{
    URI uri{"http://www.example.com/path?query#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path?query#fragment", "http",
                          "", "www.example.com", "", 0, "/path", "query", "fragment"));
}

TEST(parse_test, test_hierarchical_part_valid_host_path_and_fragment)
{
    URI uri{"http://www.example.com/path#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path#fragment", "http",
                          "", "www.example.com", "", 0, "/path", "", "fragment"));
}

TEST(parse_test, test_invalid_fragment)
{
    ASSERT_THROW(URI{"http://www.example.com/path#%fragment"}, uri_syntax_error);
}

TEST(parse_test, test_valid_fragment_with_pct_encoded_char)
{
    URI uri{"http://www.example.com/path#%ffragment"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path#%ffragment", "http",
                          "", "www.example.com", "", 0, "/path", "", "%ffragment"));
}

TEST(parse_test, test_valid_fragment_with_unreserved_char)
{
    URI uri{"http://www.example.com/path#fragment-"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path#fragment-", "http",
                          "", "www.example.com", "", 0, "/path", "", "fragment-"));
}

TEST(parse_test, test_invalid_fragment_with_gen_delim)
{
    ASSERT_THROW(URI{"http://www.example.com/path#frag#ment"}, uri_syntax_error);
}

TEST(parse_test, test_valid_fragment_with_forward_slash_and_question_mark)
{
    URI uri{"http://www.example.com/path#frag/ment?"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path#frag/ment?", "http",
                          "", "www.example.com", "", 0, "/path", "", "frag/ment?"));
}

TEST(parse_test, test_invalid_query)
{
    ASSERT_THROW(URI{"http://www.example.com/path?%query"}, uri_syntax_error);
}

TEST(parse_test, test_valid_query_with_pct_encoded_char)
{
    URI uri{"http://www.example.com/path?%00query"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path?%00query", "http",
                          "", "www.example.com", "", 0, "/path", "%00query", ""));
}

TEST(parse_test, test_valid_query_with_unreserved_char)
{
    URI uri{"http://www.example.com/path?query-"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path?query-", "http",
                          "", "www.example.com", "", 0, "/path", "query-", ""));
}

TEST(parse_test, test_valid_query_with_sub_delim)
{
    URI uri{"http://www.example.com/path?qu$ery"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com/path?qu$ery", "http",
                          "", "www.example.com", "", 0, "/path", "qu$ery", ""));
}

TEST(parse_test, test_empty_port_with_path)
{
    URI uri{"http://123.34.23.56:/"};
    ASSERT_TRUE(check_uri(uri, "http://123.34.23.56:/", "http",
                          "", "123.34.23.56", "", 0, "/", "", ""));
}

TEST(parse_test, test_empty_port)
{
    URI uri{"http://123.34.23.56:"};
    ASSERT_TRUE(check_uri(uri, "http://123.34.23.56:", "http",
                          "", "123.34.23.56", "", 0, "", "", ""));
}

TEST(parse_test, test_invalid_port_with_path)
{
    ASSERT_THROW(URI{"http://123.34.23.56:6662626/"}, uri_syntax_error);
}

TEST(parse_test, test_ipv6_address)
{
    URI uri{"http://[1080:0:0:0:8:800:200C:417A]"};
    ASSERT_TRUE(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]", "http",
                          "", "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", ""));
}

TEST(parse_test, test_ipv6_address_with_path)
{
    URI uri{"http://[1080:0:0:0:8:800:200C:417A]/"};
    ASSERT_TRUE(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]/", "http",
                          "", "[1080:0:0:0:8:800:200C:417A]", "", 0, "/", "", ""));
}

TEST(parse_test, test_invalid_ipv6_address)
{
    ASSERT_THROW(URI{"http://[1080:0:0:0:8:800:200C:417A"}, uri_syntax_error);
}

TEST(parse_test, test_invalid_ipv6_address_with_path)
{
    ASSERT_THROW(URI{"http://[1080:0:0:0:8:800:200C:417A/"}, uri_syntax_error);
}

TEST(parse_test, test_opaque_uri_with_one_slash)
{
    URI uri{"scheme:/path/"};
    ASSERT_TRUE(check_uri(uri, "scheme:/path/", "scheme", "", "", "", 0, "/path/", "", ""));
}

TEST(parse_test, test_query_with_empty_path)
{
    URI uri{"http://www.example.com?query"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com?query", "http", "", "www.example.com", "", 0, "", "query", ""));
}

TEST(parse_test, test_query_with_user_info_and_empty_path)
{
    URI uri{"http://user@www.example.com?query"};
    ASSERT_TRUE(check_uri(uri, "http://user@www.example.com?query", "http", "user",
                          "www.example.com", "", 0, "", "query", ""));
}

TEST(parse_test, test_fragment_with_empty_path)
{
    URI uri{"http://www.example.com#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://www.example.com#fragment", "http", "",
                          "www.example.com", "", 0, "", "", "fragment"));
}

TEST(parse_test, test_fragment_with_user_info_and_empty_path)
{
    URI uri{"http://user@www.example.com#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://user@www.example.com#fragment", "http", "user",
                          "www.example.com", "", 0, "", "", "fragment"));
}

TEST(parse_test, test_query_with_empty_path_and_ipv6_address)
{
    URI uri{"http://[1080:0:0:0:8:800:200C:417A]?query"};
    ASSERT_TRUE(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]?query", "http", "",
                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "query", ""));
}

TEST(parse_test, test_query_with_user_info_empty_path_and_ipv6_address)
{
    URI uri{"http://user@[1080:0:0:0:8:800:200C:417A]?query"};
    ASSERT_TRUE(check_uri(uri, "http://user@[1080:0:0:0:8:800:200C:417A]?query", "http", "user",
                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "query", ""));
}

TEST(parse_test, test_fragment_with_empty_path_and_ipv6_address)
{
    URI uri{"http://[1080:0:0:0:8:800:200C:417A]#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://[1080:0:0:0:8:800:200C:417A]#fragment", "http", "",
                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", "fragment"));
}

TEST(parse_test, test_fragment_with_user_info_empty_path_and_ipv6_address)
{
    URI uri{"http://user@[1080:0:0:0:8:800:200C:417A]#fragment"};
    ASSERT_TRUE(check_uri(uri, "http://user@[1080:0:0:0:8:800:200C:417A]#fragment", "http", "user",
                          "[1080:0:0:0:8:800:200C:417A]", "", 0, "", "", "fragment"));
}

TEST(parse_test, test_pct_encoded_user_info)
{
    URI uri{"http://user%3f@www.example.com/"};
    ASSERT_TRUE(check_uri(uri, "http://user%3f@www.example.com/", "http", "user%3f",
                          "www.example.com", "", 0, "/", "", ""));
}

TEST(parse_test, test_path_with_query_and_fragment)
{
    URI uri{"/path?query#fragment"};
    ASSERT_TRUE(check_uri(uri, "/path?query#fragment", "", "", "", "", 0, "/path", "query", "fragment"));
}

TEST(parse_test, test_path_with_query)
{
    URI uri{"/path?query"};
    ASSERT_TRUE(check_uri(uri, "/path?query", "", "", "", "", 0, "/path", "query", ""));
}

TEST(parse_test, test_path_with_fragment)
{
    URI uri{"/path#fragment"};
    ASSERT_TRUE(check_uri(uri, "/path#fragment", "", "", "", "", 0, "/path", "", "fragment"));
}

TEST(parse_test, test_path_only)
{
    URI uri{"/path"};
    ASSERT_TRUE(check_uri(uri, "/path", "", "", "", "", 0, "/path", "", ""));
}

TEST(parse_test, test_query_only)
{
    URI uri{"?query"};
    ASSERT_TRUE(check_uri(uri, "?query", "", "", "", "", 0, "", "query", ""));
}

TEST(parse_test, test_fragment_only)
{
    URI uri{"#fragment"};
    ASSERT_TRUE(check_uri(uri, "#fragment", "", "", "", "", 0, "", "", "fragment"));
}
