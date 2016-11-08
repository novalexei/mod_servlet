#ifndef MOD_SERVLET_IMPL_FILTER_CHAIN_H
#define MOD_SERVLET_IMPL_FILTER_CHAIN_H

#include <set>

#include <servlet/servlet.h>
#include <servlet/filter.h>
#include "dispatcher.h"

namespace servlet
{

class _filter_chain : public filter_chain
{
public:
    _filter_chain(const std::vector<std::shared_ptr<mapped_filter>> *url_filters,
                  const std::vector<std::shared_ptr<mapped_filter>> *name_filters, http_servlet* servlet) :
            _url_filters{url_filters}, _name_filters{name_filters}, _servlet{servlet} {}
    ~_filter_chain() noexcept override {};

    void do_filter(http_request& request, http_response& response);
private:
    const std::vector<std::shared_ptr<mapped_filter>>* _url_filters;
    const std::vector<std::shared_ptr<mapped_filter>>* _name_filters;
    std::size_t _url_pos = 0;
    std::size_t _name_pos = 0;
    std::size_t _current_order = 0;
    http_servlet* _servlet;
    std::set<http_filter*> _filter_set;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_FILTER_CHAIN_H
