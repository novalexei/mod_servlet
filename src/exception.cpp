#include <servlet/lib/exception.h>
#include "config.h"

namespace servlet
{

std::ostream &operator<<(std::ostream &out, const std::exception &ex)
{
    out << demangle(typeid(ex).name()) << ": " << ex.what() << '\n';
    /* Now let's check if this exception has nested one. */
    try { std::rethrow_if_nested(ex); }
    catch (const std::exception &nested) { out << "Caused by: " << nested; }
    catch (...) { out << "Caused by: unrecognized nested exception\n"; }
    return out;
}

} // end of servlet namespace
