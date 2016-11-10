#include <iostream>
#include <servlet/uri.h>
#include <servlet/lib/any_map.h>
#include "../src/time.h"

using namespace servlet;

int main()
{
    try
    {
        std::string lib_subpath{"lib.so(/other/context)"};
        if (lib_subpath.back() != ')')
        {
            std::cout << "WEB-INF/lib/" << lib_subpath << std::endl;
        }
        else
        {
            std::string::size_type open_bracket = lib_subpath.find('(');
            if (open_bracket == std::string::npos) throw config_exception{"Invalid library name: '" + lib_subpath + "'"};
            string_view lib_name = string_view{lib_subpath}.substr(0, open_bracket);
            int offset = lib_subpath[open_bracket+1] == '/' ? 2 : 1;
            int back_offset = lib_subpath[lib_subpath.length()-2] == '/' ? 2 : 1;
            std::string webapp_name = lib_subpath.substr(open_bracket+offset,
                                                         lib_subpath.length()-open_bracket-offset-back_offset);
            std::replace(webapp_name.begin(), webapp_name.end(), '/', '#');
            std::cout << "/" << webapp_name << "/WEB-INF/lib/" << lib_name << std::endl;
//            std::cout << lib_name << std::endl;
//            std::cout << webapp_name << std::endl;
        }

//        std::map<std::string, std::string> name_value_map;
//        std::string query{"my%20name=my%20value&no%20value&another%20no%20value="};
//        URI::parse_query(query, [&name_value_map] (const std::string& name, const std::string& value)
//        {
//            name_value_map.emplace(std::move(name), std::move(value));
//        });
//        for (auto &&item : name_value_map)
//        {
//            std::cout << "\"" << item.first << "\" -> \"" << item.second << "\"" << std::endl;
//        }
//        std::cout << URI::decode("my%20name=my%20value") << std::endl;
    }
    catch(std::exception& e) { std::cout << e << std::endl; }
}
