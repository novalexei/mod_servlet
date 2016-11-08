/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_SSL_H
#define MOD_SERVLET_IMPL_SSL_H

#include <servlet/ssl.h>

namespace servlet
{

class certificate_impl : public certificate
{
public:
    certificate_impl(const std::map<string_view, string_view, std::less<>>& env, string_view prefix);

    int version() const override { return _version; }
    string_view serial_number() const override { return _serial; }
    time_type valid_since() const override { return _valid_since; }
    time_type valid_until() const override { return _valid_until; }
    bool check_valid() const override { return check_valid(std::chrono::system_clock::now()); }
    bool check_valid(time_type time) const override { return time <= _valid_until && time >= _valid_since; }
    string_view signature_algorithm_name() const override { return _sig_alg; }
    string_view key_algorithm_name() const override { return _key_alg; }
    string_view subject_DN() const override { return _s_DN; }
    const std::map<string_view, string_view, std::less<>>& subject_DN_components() const override { return _s_DN_n; }
    string_view issuer_DN() const override { return _i_DN; }
    const std::map<string_view, string_view, std::less<>>& issuer_DN_components() const override { return _i_DN_n; }

    const std::map<string_view, std::vector<string_view>, std::less<>>& subject_alternative_names() const override
    { return _san; }

    string_view certificate_exact_assertion() const override { return _cea; }
    const std::vector<string_view>& certificate_chain() const override { return _chain; }

    string_view PEM_encoded() const override { return _cert; }
private:
    int _version = 0;
    string_view _serial;
    time_type _valid_since;
    time_type _valid_until;
    string_view _sig_alg;
    string_view _key_alg;
    string_view _s_DN;
    string_view _i_DN;
    string_view _cea;
    string_view _cert;

    std::map<string_view, string_view, std::less<>> _s_DN_n;
    std::map<string_view, string_view, std::less<>> _i_DN_n;
    std::map<string_view, std::vector<string_view>, std::less<>> _san;
    std::vector<string_view> _chain;
};

class SSL_info : public SSL_information
{
public:
    SSL_info(const std::map<string_view, string_view, std::less<>>& env);

    string_view protocol() const override { return _protocol; }
    string_view cipher_name() const override { return _cipher; }
    bool is_cipher_export() const override { return _cipher_export; }
    int cipher_used_bits() const override { return _cipher_used_bits; }
    int cipher_possible_bits() const override { return _cipher_possible_bits; }
    string_view compress_method() const override { return _compress_method; }
    string_view session_id() const override { return _session_id; }
    SSL_SESSION_STATE session_state() const override { return _session_state; }

    const certificate& client_certificate() const override { return _client_cert; }
    const certificate& server_certificate() const override { return _server_cert; }
private:
    string_view _protocol;
    string_view _cipher;
    bool _cipher_export;
    int _cipher_used_bits;
    int _cipher_possible_bits;
    string_view _compress_method;
    string_view _session_id;
    SSL_SESSION_STATE _session_state = SSL_SESSION_STATE::INITIAL;

    certificate_impl _client_cert;
    certificate_impl _server_cert;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_SSL_H
