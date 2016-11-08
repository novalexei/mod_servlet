#include <iostream>
#include <servlet/uri.h>
#include <servlet/lib/any_map.h>
#include "../src/time.h"

using namespace servlet;

int main()
{
    try
    {
        std::map<std::string, std::string> name_value_map;
        std::string query{"my%20name=my%20value&no%20value&another%20no%20value="};
        URI::parse_query(query, [&name_value_map] (const std::string& name, const std::string& value)
        {
            name_value_map.emplace(std::move(name), std::move(value));
        });
        for (auto &&item : name_value_map)
        {
            std::cout << "\"" << item.first << "\" -> \"" << item.second << "\"" << std::endl;
        }
//        std::cout << URI::decode("my%20name=my%20value") << std::endl;
    }
    catch(std::exception& e) { std::cout << e << std::endl; }
}
