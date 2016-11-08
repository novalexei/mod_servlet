#include <gtest/gtest.h>
#include <servlet/uri.h>

using namespace servlet;

TEST(res_norm_test, double_resolve_test)
{
    URI base{"http://java.sun.com/j2se/1.3/index.html"};
    URI relative1{"docs/guide/collections/designfaq.html#28"};
    URI relative2{"../../../demo/jfc/SwingSet2/src/SwingSet2.java"};
    URI resolved1 = base.resolve(relative1);
    URI resolved2 = resolved1.resolve(relative2);
    ASSERT_EQ(resolved1.uri_view(), "http://java.sun.com/j2se/1.3/docs/guide/collections/designfaq.html#28");
    ASSERT_EQ(resolved2.uri_view(), "http://java.sun.com/j2se/1.3/demo/jfc/SwingSet2/src/SwingSet2.java");
    URI relativized2 = resolved1.relativize(resolved2);
    URI relativized1 = base.relativize(resolved1);
    ASSERT_EQ(relativized2, relative2);
    ASSERT_EQ(relativized1, relative1);
}

TEST(res_norm_test, absolute_resolve_test)
{
    URI base{"http://java.sun.com/j2se/1.3/index.html"};
    URI relative{"file:///~calendar"};
    URI resolved = base.resolve(relative);
    ASSERT_EQ(resolved.uri_view(), "file:///~calendar");
}

TEST(res_norm_test, relative_resolve_test)
{
    URI base{"docs/guide/collections/designfaq.html#28"};
    URI relative{"../../../demo/jfc/SwingSet2/src/SwingSet2.java"};
    URI resolved = base.resolve(relative);
    ASSERT_EQ(resolved.uri_view(), "demo/jfc/SwingSet2/src/SwingSet2.java");
}

TEST(res_norm_test, simple_relativize_test)
{
    URI base{"http://java.sun.com/j2se/1.3/"};
    URI relative{"http://java.sun.com/j2se/1.3/docs/guide/index.html"};
    URI resolved = base.relativize(relative);
    ASSERT_EQ(resolved.uri_view(), "docs/guide/index.html");
}
