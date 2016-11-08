#include <limits>
#include <thread>
#include <random>
#include <mutex>

#include "session.h"

namespace servlet
{

class system_random
{
public:
    typedef std::uniform_int_distribution<int> distr_type;
    system_random() :
            _gen{static_cast<std::mt19937_64::result_type>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())} {}

    typename distr_type::result_type operator()() { return _distr(_gen); }

private:
    std::mt19937_64 _gen;
    distr_type _distr{std::numeric_limits<unsigned char>::min(), std::numeric_limits<unsigned char>::max()};
};

void random_bytes(std::array<unsigned char, 16> &bytes)
{
    static system_random BYTES_RANDOM;
    static std::mutex BYTES_MX;
    std::lock_guard<std::mutex> lk{BYTES_MX};
    for (auto &byte : bytes) byte = static_cast<unsigned char>(BYTES_RANDOM());
}

std::string generate_session_id()
{
    std::array<unsigned char, 16> random;

    std::string buffer{};
    int resultLenBytes = 0;
    while (resultLenBytes < 16)
    {
        random_bytes(random);
        for (int j = 0; j < 16 && resultLenBytes < 16; ++j)
        {
            unsigned char b1 = static_cast<unsigned char>((random[j] & 0xf0) >> 4);
            unsigned char b2 = static_cast<unsigned char>(random[j] & 0x0f);
            if (b1 < 10) buffer.append(1, '0' + b1);
            else buffer.append(1, static_cast<char>('A' + (b1 - 10)));
            if (b2 < 10) buffer.append(1, '0' + b2);
            else buffer.append(1, static_cast<char>('A' + (b2 - 10)));
            ++resultLenBytes;
        }
    }
    return buffer;
}

http_session::http_session(const string_view &client_ip, const string_view &user_agent) :
        _session_id{generate_session_id()}, _client_ip{client_ip.to_string()}, _user_agent{user_agent.to_string()},
        _created{std::chrono::system_clock::now()}, _last_accessed{std::chrono::system_clock::now()} {}


void http_session_impl::validate(const string_view &client_ip, const string_view &user_agent)
{
    if (_client_ip != client_ip)
        throw security_exception{"session was requested by a user with different IP"};
    if (_user_agent != user_agent)
        throw security_exception{"session was requested by a user with different User-Agent"};
    _new = false;
    _last_accessed = std::chrono::system_clock::now();
}

} // end of servlet namespace
