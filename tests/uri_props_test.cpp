#define BOOST_TEST_MODULE uri_props_test
#include <boost/test/included/unit_test.hpp>
#include <servlet/uri.h>

using namespace servlet;

//BOOST_AUTO_TEST_CASE(mail_uri_props_test)
//{
//    URI uri{"mailto:mduerst@ifi.unizh.ch"};
//    BOOST_CHECK(uri.is_absolute());
//    BOOST_CHECK(uri.is_opaque());
//}
//
//BOOST_AUTO_TEST_CASE(news_uri_props_test)
//{
//    URI uri{"news:comp.lang.java"};
//    BOOST_CHECK(uri.is_absolute());
//    BOOST_CHECK(uri.is_opaque());
//}
//
//BOOST_AUTO_TEST_CASE(http_uri_props_test)
//{
//    URI uri{"http://localhost"};
//    BOOST_CHECK(uri.is_absolute());
//    BOOST_CHECK(!uri.is_opaque());
//}
