/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include "logger_format.h"

namespace servlet { namespace logging {

/*
 * Do we need to make cache size configurable?
 */
ptr_cache<inplace_ostream> INPLACE_STRING_STREAM_CACHE{new string_ptr_provider{}, 32};

static std::string __absolute_path(const std::string &log_file, const std::string &base_dir)
{
    if (base_dir.empty()) return log_file;
    else
    {
        fs::path log{log_file};
        if (log.is_absolute()) return log_file;
        else
        {
            fs::path base_path{base_dir};
            return (base_path / log).string();
        }
    }
}

void _log_messages_consumer::operator()(inplace_ostream *str)
{
    if (!str) _running = false;
    else
    {
        inplace_ostream_cache_returner pr{str};
        _log_output->write_string((*str)->view());
        if ((*str)->was_flushed()) _log_output->flush();
    }
}

async_locked_stream::~async_locked_stream() noexcept
{
    try
    {
        _async_queue->push(nullptr);
        _consumer_thread.join();
    }
    catch (const std::exception &e)
    {
        std::cout << "Failure to close async logging consumer: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Failure to close async logging consumer: unknown exception";
    }
}

void size_rotation_file_log_output::_check_file()
{
    if (_out->count() >= _max_size)
    {
        _out->stream().close();
        _out = counted_ofstream{_fn_ctor->get_name(_cur_number), std::ios_base::out | std::ios_base::trunc};
        ++_cur_number;
    }
}

static bool _add_date_element(std::vector<std::string> &parts, const std::string &fmt, std::string::size_type &cur_pos,
                              std::string::size_type fnd_idx, std::vector<std::string>::difference_type &part_idx)
{
    parts.push_back(fmt.substr(cur_pos, fnd_idx - cur_pos));
    parts.push_back(std::string{});
    part_idx = parts.size() - 1;
    cur_pos = fnd_idx + 2;
    return true;
}

static file_name_constructor *_create_name_ctor(const std::string& log_file, const std::string& base_dir,
                                                bool force_size = false, bool force_date = false)
{
    namespace fs = std::experimental::filesystem;
    fs::path out_path{log_file};
    if (out_path.is_relative())
    {
        if (base_dir.empty()) out_path = fs::absolute(out_path);
        else out_path = fs::absolute(out_path, fs::path{base_dir});
    }
    out_path.make_preferred();
    std::string fn = out_path.filename().string();
    std::string dir_path = out_path.string();
    std::vector<std::string> parts{};
    parts.push_back(dir_path.substr(0, dir_path.size() - fn.size()));
    std::vector<std::string> fparts{};

    bool has_date = false;
    bool has_size = false;

    std::vector<std::string>::difference_type y_idx = -1, Y_idx = -1, m_idx = -1, d_idx = -1, n_idx = -1;
    int n_width = 0;
    std::string::size_type cur_pos = 0;
    while (cur_pos != std::string::npos && cur_pos < fn.size())
    {
        std::string::size_type idx = fn.find('%', cur_pos);
        if (idx == std::string::npos || idx == fn.size() - 1)
        {
            fparts.push_back(fn.substr(cur_pos));
            break;
        }
        if (fn[idx + 1] == 'y') has_date = _add_date_element(fparts, fn, cur_pos, idx, y_idx);
        else if (fn[idx + 1] == 'Y') has_date = _add_date_element(fparts, fn, cur_pos, idx, Y_idx);
        else if (fn[idx + 1] == 'm') has_date = _add_date_element(fparts, fn, cur_pos, idx, m_idx);
        else if (fn[idx + 1] == 'd') has_date = _add_date_element(fparts, fn, cur_pos, idx, d_idx);
        else if (fn[idx + 1] == 'N')
        {
            std::string::size_type next_idx = fn.find('%', idx + 1);
            if (next_idx == std::string::npos)
            {
                fparts.push_back(fn.substr(cur_pos));
                break;
            }
            std::string n_str = fn.substr(idx + 1, next_idx - idx - 1);
            if (n_str.find_first_not_of('N') == std::string::npos)
            {
                has_size = true;
                fparts.push_back(fn.substr(cur_pos, idx - cur_pos));
                fparts.push_back(std::string{});
                n_idx = fparts.size() - 1;
                n_width = n_str.size();
                cur_pos = next_idx + 1;
            }
            else
            {
                fparts.push_back(fn.substr(cur_pos, idx - cur_pos + 1));
                cur_pos = idx + 1;
            }
        }
        else
        {
            fparts.push_back(fn.substr(cur_pos, idx - cur_pos + 1));
            cur_pos = idx + 1;
        }
    }
    auto parts_size = parts.size();
    if (!has_date && force_date)
    {
        parts.push_back(std::string{});
        y_idx = 0; /* %y */
        parts.push_back(std::string{"-"});
        parts.push_back(std::string{});
        m_idx = 2; /* %m */
        parts.push_back(std::string{"-"});
        parts.push_back(std::string{});
        d_idx = 4; /* %d */
        parts.push_back(std::string{"."});
    }
    std::copy(fparts.begin(), fparts.end(), std::back_inserter(parts));
    if (y_idx != -1) y_idx += parts_size;
    if (Y_idx != -1) Y_idx += parts_size;
    if (m_idx != -1) m_idx += parts_size;
    if (d_idx != -1) d_idx += parts_size;
    if (n_idx != -1) n_idx += parts_size;
    if (!has_size && force_size)
    {
        parts.push_back(std::string{"."});
        parts.push_back(std::string{}); /* %NN% */
        n_idx = parts.size() - 1;
        n_width = 2;
    }
    return new rotating_file_name_constructor{parts, y_idx, Y_idx, m_idx, d_idx, n_idx, n_width};
}

void size_rotation_file_log_output::load_config(std::map<std::string, std::string, std::less<>>& props,
                                                const std::string& conf_prefix, const std::string &base_dir)
{
    auto it = props.find(conf_prefix.empty() ? "log.file" : conf_prefix + "log.file");
    std::string log_file;
    if (it != props.end() && !it->second.empty()) log_file = std::move(_trim(it->second).to_string());
    if (log_file.empty()) log_file = "app.log";
    it = props.find(conf_prefix.empty() ? "rotation.size" : conf_prefix + "rotation.size");
    if (it != props.end() && !it->second.empty())
        _max_size = from_string(_trim(it->second), log_registry::DEFAULT_FILE_ROTATION_SIZE);
    _fn_ctor = _create_name_ctor(log_file, base_dir, true);
    _out->stream().open(_fn_ctor->get_name(0), std::ios_base::out | std::ios_base::trunc);
}

void date_rotation_file_log_output::_check_file()
{
    _ts = std::chrono::system_clock::now();
    if (_ts > _tomorrow)
    {
        _out.close();
        _out = std::ofstream{_fn_ctor->get_name(_ts), std::ios_base::out | std::ios_base::trunc};
        _tomorrow = tomorrow(_ts);
    }
}

void date_rotation_file_log_output::load_config(std::map<std::string, std::string, std::less<>>& props,
                                                const std::string& conf_prefix, const std::string &base_dir)
{
    auto it = props.find(conf_prefix.empty() ? "log.file" : conf_prefix + "log.file");
    std::string log_file;
    if (it != props.end() && !it->second.empty()) log_file = std::move(_trim(it->second).to_string());
    if (log_file.empty()) log_file = "app.log";
    _fn_ctor = _create_name_ctor(log_file, base_dir, false, true);
    _out.open(_fn_ctor->get_name(_ts), std::ios_base::out | std::ios_base::trunc);
}

void date_size_rotation_file_log_output::_check_file()
{
    _ts = std::chrono::system_clock::now();
    if (_ts > _tomorrow)
    {
        _out->stream().close();
        _cur_number = 0;
        _out = counted_ofstream{_fn_ctor->get_name(_ts, _cur_number),
                                std::ios_base::out | std::ios_base::trunc};
        _tomorrow = tomorrow(_ts);
    }
    if (_out->count() >= _max_size)
    {
        _out->stream().close();
        _out = counted_ofstream{_fn_ctor->get_name(_ts, _cur_number), std::ios_base::out | std::ios_base::trunc};
        ++_cur_number;
    }
}

void date_size_rotation_file_log_output::load_config(std::map<std::string, std::string, std::less<>> &props,
                                                     const std::string &conf_prefix, const std::string &base_dir)
{
    auto it = props.find(conf_prefix.empty() ? "log.file" : conf_prefix + "log.file");
    std::string log_file;
    if (it != props.end() && !it->second.empty()) log_file = std::move(_trim(it->second).to_string());
    if (log_file.empty()) log_file = "app.log";
    it = props.find(conf_prefix.empty() ? "rotation.size" : conf_prefix + "rotation.size");
    if (it != props.end() && !it->second.empty())
        _max_size = from_string(_trim(it->second), log_registry::DEFAULT_FILE_ROTATION_SIZE);
    _fn_ctor = _create_name_ctor(log_file, base_dir, true, true);
    _out->stream().open(_fn_ctor->get_name(_ts, _cur_number), std::ios_base::out | std::ios_base::trunc);
}

char const *simple_prefix_printer::LEVEL_STR[] = {"CRT", "ERR", "WRN", "INF", "CFG", "DBG", "TRC"};

void simple_prefix_printer::print_prefix(LEVEL level, const std::string &name, std::ostream &out) const
{
    if (!_prefix.empty()) out << _prefix;
    if (_prefix_format.empty()) return;
    for (auto& prefix_element : _prefix_format)
    {
        switch (prefix_element.first)
        {
            case PREFIX_VALUE::THREAD_ID:
                out << std::this_thread::get_id() << prefix_element.second;
                break;
            case PREFIX_VALUE::TIMESTAMP:
                if (_tstamp_fmt) out << (*_tstamp_fmt)(std::chrono::system_clock::now()) << prefix_element.second;
                else out << std::chrono::system_clock::now() << prefix_element.second;
                break;
            case PREFIX_VALUE::LEVEL:
                out << LEVEL_STR[static_cast<int>(level)] << prefix_element.second;
                break;
            case PREFIX_VALUE::NAME:
                out << name << prefix_element.second;
                break;
        }
    }
}

inline static bool set_format_element(std::string::size_type read_index, std::string::size_type found_index,
                                      const std::string &line_format, const std::string &token,
                                      std::string *&cur_str, PREFIX_VALUE val_to_set, bool push,
                                      std::pair<PREFIX_VALUE, std::string> &cur_pair,
                                      std::vector<std::pair<PREFIX_VALUE, std::string>> &fmt_vec)
{
    if (line_format.compare(found_index, token.size(), token) != 0) return false;
    cur_str->append(line_format.substr(read_index, found_index - read_index));
    if (push)
    {
        fmt_vec.push_back(cur_pair);
        cur_str->clear();
    }
    else cur_str = &cur_pair.second;
    cur_pair.first = val_to_set;
    return true;
}

void simple_prefix_printer::load_config(std::map<std::string, std::string, std::less<>>& props,
                                        const std::string& conf_prefix)
{
    /* parse timestamp format */
    auto it = props.find(conf_prefix.empty() ? "timestamp.format" : conf_prefix + "timestamp.format");
    if (it != props.end())
    {
        std::string timestamp_fmt{std::move(_trim(it->second).to_string())};
        if (!timestamp_fmt.empty()) _tstamp_fmt = new time_point_format{timestamp_fmt};
    }
    /* parse prefix format */
    static std::string TIMESTAMP_TOKEN{"%TIME%"};
    static std::string THREAD_ID_TOKEN{"%THREAD%"};
    static std::string NAME_TOKEN{"%NAME%"};
    static std::string LEVEL_TOKEN{"%LEVEL%"};
    it = props.find(conf_prefix.empty() ? "log.prefix.format" : conf_prefix + "log.prefix.format");
    if (it == props.end()) return;
    std::string line_format{_trim(it->second).to_string()};
    _prefix_format.clear();
    _prefix.clear();
    std::string *cur_str = &_prefix;
    bool push = false;
    std::pair<PREFIX_VALUE, std::string> cur_pair;
    std::string::size_type read_index = 0;
    while (true)
    {
        std::string::size_type found = line_format.find('%', read_index);
        if (found == std::string::npos)
        {
            cur_str->append(line_format.substr(read_index));
            if (push) _prefix_format.push_back(cur_pair);
            break;
        }
        if (set_format_element(read_index, found, line_format, TIMESTAMP_TOKEN, cur_str,
                               PREFIX_VALUE::TIMESTAMP, push, cur_pair, _prefix_format))
        {
            push = true;
            read_index = found + TIMESTAMP_TOKEN.size();
        }
        else if (set_format_element(read_index, found, line_format, THREAD_ID_TOKEN, cur_str,
                                    PREFIX_VALUE::THREAD_ID, push, cur_pair, _prefix_format))
        {
            push = true;
            read_index = found + THREAD_ID_TOKEN.size();
        }
        else if (set_format_element(read_index, found, line_format, NAME_TOKEN, cur_str,
                                    PREFIX_VALUE::NAME, push, cur_pair, _prefix_format))
        {
            push = true;
            read_index = found + NAME_TOKEN.size();
        }
        else if (set_format_element(read_index, found, line_format, LEVEL_TOKEN, cur_str,
                                    PREFIX_VALUE::LEVEL, push, cur_pair, _prefix_format))
        {
            push = true;
            read_index = found + LEVEL_TOKEN.size();
        }
        else
        {
            cur_str->append(line_format.substr(read_index, found - read_index + 1));
            read_index = found + 1;
        }
    }
}

void rotating_file_name_constructor::_set_number(int next_num)
{
    if (_n_idx < 0) return;
    std::string &n_str = _name_parts[_n_idx];
    n_str.clear(); n_str << setpad(next_num, _n_width < 2 ? 2 : _n_width, '0');
}
void rotating_file_name_constructor::_set_date_part(idx_type idx, int width, int value)
{
    if (idx >= 0)
    {
        std::string &str = _name_parts[idx];
        str.clear(); str << setpad(value, width, '0');
    }
}
void rotating_file_name_constructor::_set_date(_tp_type tp)
{
    const auto epoch = tp.time_since_epoch();
    const std::time_t s = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(epoch).count());
    const std::tm tmv = get_tm(s);
    _set_date_part(_y_idx, 2, tmv.tm_year % 100);
    _set_date_part(_Y_idx, 4, tmv.tm_year + 1900);
    _set_date_part(_m_idx, 2, tmv.tm_mon + 1);
    _set_date_part(_d_idx, 2, tmv.tm_mday);
}

std::string rotating_file_name_constructor::_compose_name() const
{
    std::string res{};
    for (auto &&part : _name_parts) res += part;
    return res;
}

} // end of servlet::logging namespace

} // end of servlet namespace
