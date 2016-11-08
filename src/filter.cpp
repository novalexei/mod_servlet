#include <servlet/filter.h>
#include "dispatcher.h"

#include "filter_chain.h"

namespace servlet
{

void http_filter::init(filter_config& cfg)
{
    _cfg = &cfg;
    init();
}

static http_filter* _get_filter(const std::vector<std::shared_ptr<mapped_filter>>* url_filters, std::size_t& url_pos,
                                const std::vector<std::shared_ptr<mapped_filter>>* name_filters, std::size_t& name_pos)
{
    http_filter *filter = nullptr;
    if (url_filters && url_pos < url_filters->size())
    {
        if (name_filters && name_pos < name_filters->size())
        {
            if ((*url_filters)[url_pos]->get_order() < (*name_filters)[name_pos]->get_order())
            {
                filter = (*url_filters)[url_pos]->get_filter();
                ++url_pos;
            }
            else
            {
                filter = (*name_filters)[name_pos]->get_filter();
                ++name_pos;
            }
        }
        else
        {
            filter = (*url_filters)[url_pos]->get_filter();
            ++url_pos;
        }
    }
    else if (name_filters && name_pos < name_filters->size())
    {
        filter = (*name_filters)[name_pos]->get_filter();
        ++name_pos;
    }
    return filter;
}

void _filter_chain::do_filter(http_request& request, http_response& response)
{
    http_filter *filter;
    do { filter = _get_filter(_url_filters, _url_pos, _name_filters, _name_pos); }
    while (filter && !_filter_set.insert(filter).second);
    if (filter) filter->do_filter(request, response, *this);
    else _servlet->service(request, response);
}

} // end of servlet namespace
