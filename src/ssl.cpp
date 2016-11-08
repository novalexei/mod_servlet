#include "ssl.h"
#include "config.h"
#include "time.h"

namespace servlet
{

static inline certificate::time_type _parse_time(string_view time_str)
{
    std::tm t;
    string_view_istream in{time_str};
    in.imbue(std::locale::classic());
    in >> std::get_time(&t, "%b %d %H:%M:%S %Y");
    std::time_t epoch = std::mktime(&t); /* This epoch time is local, we need to convert it to GMT */

    t = get_gmtm(epoch);
    auto gm_diff = std::mktime(&t) - epoch;
    epoch -= gm_diff;

    return std::chrono::system_clock::from_time_t(epoch);
}

certificate_impl::certificate_impl(const std::map<string_view, string_view, std::less<>>& env, string_view prefix)
{
    auto lg = servlet_logger();
    for (auto &item : env)
    {
        if (!begins_with(item.first, prefix)) continue;
        lg->config() << "certificate property: " << item.first << " -> " << item.second << '\n';
        string_view suffix = item.first.substr(prefix.length());
        if (suffix == "M_VERSION") item.second >> _version;
        else if (suffix == "M_VERSION") _serial = item.second;
        else if (suffix == "A_SIG") _sig_alg = item.second;
        else if (suffix == "A_KEY") _key_alg = item.second;
        else if (suffix == "S_DN") _s_DN = item.second;
        else if (suffix == "I_DN") _i_DN = item.second;
        else if (begins_with(suffix, "S_DN_")) _s_DN_n.emplace(suffix.substr(5), item.second);
        else if (begins_with(suffix, "I_DN_")) _i_DN_n.emplace(suffix.substr(5), item.second);
        else if (begins_with(suffix, "CERT_CHAIN_")) _chain.push_back(item.second);
        else if (suffix == "CERT") _cert = item.second;
        else if (suffix == "CERT_RFC4523_CEA") _cea = item.second;
            /* It seams that mod_ssl populates date fields with strings in format "%b" */
        else if (suffix == "V_START") _valid_since = _parse_time(item.second);
        else if (suffix == "V_END") _valid_until = _parse_time(item.second);
    }
}

SSL_info::SSL_info(const std::map<string_view, string_view, std::less<>> &env) :
        _client_cert{env, "SSL_CLIENT_"}, _server_cert{env, "SSL_SERVER_"}
{
    for (auto &item : env)
    {
        if (item.first == "SSL_PROTOCOL") _protocol = item.second;
        else if (item.first == "SSL_CIPHER") _cipher = item.second;
        else if (item.first == "SSL_CIPHER_EXPORT") _cipher_export = equal_ic(item.second, "true");
        else if (item.first == "SSL_CIPHER_USEKEYSIZE") string_view_istream{item.second} >> _cipher_used_bits;
        else if (item.first == "SSL_CIPHER_ALGKEYSIZE") string_view_istream{item.second} >> _cipher_possible_bits;
        else if (item.first == "SSL_COMPRESS_METHOD") _compress_method = item.second;
        else if (item.first == "SSL_SESSION_ID") _session_id = item.second;
        else if (item.first == "SSL_SESSION_RESUMED")
        {
            _session_state = equal_ic(item.second, "Resumed") ? SSL_SESSION_STATE::RESUMED : SSL_SESSION_STATE::INITIAL;
        }
    }
}

} // end of servlet namespace
