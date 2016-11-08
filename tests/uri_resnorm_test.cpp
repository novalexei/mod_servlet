#define BOOST_TEST_MODULE uri_resnorm_test
#include <boost/test/included/unit_test.hpp>
#include <servlet/uri.h>

using namespace servlet;

//BOOST_AUTO_TEST_CASE(double_resolve_test)
//{
//    URI base{"http://java.sun.com/j2se/1.3/index.html"};
//    URI relative1{"docs/guide/collections/designfaq.html#28"};
//    URI relative2{"../../../demo/jfc/SwingSet2/src/SwingSet2.java"};
//    URI resolved1 = base.resolve(relative1);
//    URI resolved2 = resolved1.resolve(relative2);
//    BOOST_CHECK(resolved1.uri_view() == "http://java.sun.com/j2se/1.3/docs/guide/collections/designfaq.html#28");
//    BOOST_CHECK(resolved2.uri_view() == "http://java.sun.com/j2se/1.3/demo/jfc/SwingSet2/src/SwingSet2.java");
//    URI relativized2 = resolved1.relativize(resolved2);
//    URI relativized1 = base.relativize(resolved1);
//    BOOST_CHECK_EQUAL(relativized2, relative2);
//    BOOST_CHECK_EQUAL(relativized1, relative1);
//}
//
//BOOST_AUTO_TEST_CASE(abslute_resolve_test)
//{
//    URI base{"http://java.sun.com/j2se/1.3/index.html"};
//    URI relative{"file:///~calendar"};
//    URI resolved = base.resolve(relative);
//    BOOST_CHECK(resolved.uri_view() == "file:///~calendar");
//}
//
//BOOST_AUTO_TEST_CASE(relative_resolve_test)
//{
//    URI base{"docs/guide/collections/designfaq.html#28"};
//    URI relative{"../../../demo/jfc/SwingSet2/src/SwingSet2.java"};
//    URI resolved = base.resolve(relative);
//    BOOST_CHECK(resolved.uri_view() == "demo/jfc/SwingSet2/src/SwingSet2.java");
//}
//
//BOOST_AUTO_TEST_CASE(simple_relativize_test)
//{
//    URI base{"http://java.sun.com/j2se/1.3/"};
//    URI relative{"http://java.sun.com/j2se/1.3/docs/guide/index.html"};
//    URI resolved = base.relativize(relative);
//    BOOST_CHECK(resolved.uri_view() == "docs/guide/index.html");
//}
