/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_PROPERTIES_H
#define SERVLET_PROPERTIES_H

#include <initializer_list>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <utility>

#include "string.h"
#include <servlet/lib/optional.h>

namespace servlet
{

class properties_file : public std::map<std::string, std::string, std::less<>>
{
public:
    typedef std::map<std::string, std::string> map_type;
    typedef typename map_type::iterator        iterator;

    properties_file() = default;
    properties_file(const properties_file &props) = default;
    properties_file(properties_file &&props) = default;
    properties_file(std::istream &in) { load(in); }

    properties_file& operator=(const properties_file& other) = default;
    properties_file& operator=(properties_file&& other) = default;

    properties_file(const char *file_name)
    {
        std::ifstream in = std::ifstream{file_name};
        load(in);
    }
    properties_file(const std::string &file_name)
    {
        std::ifstream in = std::ifstream{file_name};
        load(in);
    }

    void load(std::istream &in);

    inline void list(std::ostream &out) const { for (auto &&pr : *this) out << pr.first << '=' << pr.second << '\n'; }
    inline bool hasProperty(std::string &key) const { return find(key) != end(); }

    template<typename KeyType>
    optional_ref<const std::string> get(const KeyType& key) const
    {
        const_iterator it = this->find(key);
        return it == this->end() ? optional_ref<const std::string>{} : optional_ref<const std::string>{it->second};
    }

    inline void set(const std::string &key, const std::string &value) { emplace(key, value); }
};

std::istream &operator>>(std::istream &in, properties_file &props);
std::ostream &operator<<(std::ostream &out, properties_file &props);

inline wchar_t read_wchar(const char *&from, const char *from_end);

} // end of servlet namespace

#endif // SERVLET_PROPERTIES_H
