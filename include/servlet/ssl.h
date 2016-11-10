/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_SSL_H
#define MOD_SERVLET_SSL_H

#include <vector>
#include <chrono>
#include <map>
#include <experimental/string_view>

namespace servlet
{

using std::experimental::string_view;

/**
 * Abstract class holding available information regarding client or server
 * security certificate.
 *
 * @see SSL_information::client_certificate
 * @see SSL_information::server_certificate
 */
class certificate
{
public:
    /**
     * time_point type to be used for date representation in this class
     */
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> time_type;

    /**
     * Destructor
     */
    virtual ~certificate() noexcept {};

    /**
     * Gets the <code>version</code> (version number) value from the
     * certificate.
     * The ASN.1 definition for this is:
     * <pre>
     * version  [0] EXPLICIT Version DEFAULT v1<p>
     * Version ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     * </pre>
     * @return the version number, i.e. 1, 2 or 3.
     */
    virtual int version() const = 0;

    /**
     * Gets the <code>serialNumber</code> value from the certificate.
     *
     * The serial number is an integer assigned by the certification
     * authority to each certificate. It must be unique for each
     * certificate issued by a given CA (i.e., the issuer name and
     * serial number identify a unique certificate).
     * The ASN.1 definition for this is:
     * <pre>
     * serialNumber     CertificateSerialNumber<p>
     *
     * CertificateSerialNumber  ::=  INTEGER
     * </pre>
     *
     * @return the serial number.
     */
    virtual string_view serial_number() const = 0;

    /**
     * Gets the <code>notBefore</code> date from the validity period of
     * the certificate.
     *
     * The relevant ASN.1 definitions are:
     * <pre>
     * validity             Validity<p>
     *
     * Validity ::= SEQUENCE {
     *     notBefore      CertificateValidityDate,
     *     notAfter       CertificateValidityDate }<p>
     * CertificateValidityDate ::= CHOICE {
     *     utcTime        UTCTime,
     *     generalTime    GeneralizedTime }
     * </pre>
     *
     * @return the start date of the validity period.
     * @see #check_valid
     * @see #valid_until
     */
    virtual time_type valid_since() const = 0;

    /**
     * Gets the <code>notAfter</code> date from the validity period of
     * the certificate.
     *
     * The relevant ASN.1 definitions are:
     * <pre>
     * validity             Validity<p>
     *
     * Validity ::= SEQUENCE {
     *     notBefore      CertificateValidityDate,
     *     notAfter       CertificateValidityDate }<p>
     * CertificateValidityDate ::= CHOICE {
     *     utcTime        UTCTime,
     *     generalTime    GeneralizedTime }
     * </pre>
     *
     * @return the end date of the validity period.
     * @see #check_valid
     * @see #valid_since
     */
    virtual time_type valid_until() const = 0;

    /**
     * Checks that the certificate is currently valid. It is if
     * the current date and time are within the validity period given in the
     * certificate.
     * <p>
     * The validity period consists of two date/time values:
     * the first and last dates (and times) on which the certificate
     * is valid. It is defined in
     * ASN.1 as:
     * <pre>
     * validity             Validity<p>
     * Validity ::= SEQUENCE {
     *     notBefore      CertificateValidityDate,
     *     notAfter       CertificateValidityDate }<p>
     * CertificateValidityDate ::= CHOICE {
     *     utcTime        UTCTime,
     *     generalTime    GeneralizedTime }
     * </pre>
     *
     * @return <code>true</code> if certificate is valid
     */
    virtual bool check_valid() const = 0;

    /**
     * Checks that the given date is within the certificate's
     * validity period.
     *
     * <p>In other words, this determines whether the
     * certificate would be valid at the given date/time.</p>
     *
     * @param time the Date to check against to see if this certificate
     *        is valid at that date/time.
     * @see #check_valid()
     */
    virtual bool check_valid(time_type time) const = 0;

    /**
     * Gets the signature algorithm name for the certificate
     * signature algorithm.
     *
     * An example is the string "SHA-1/DSA".
     * The ASN.1 definition for this is:
     * <pre>
     * signatureAlgorithm   AlgorithmIdentifier<p>
     * AlgorithmIdentifier  ::=  SEQUENCE  {
     *     algorithm               OBJECT IDENTIFIER,
     *     parameters              ANY DEFINED BY algorithm OPTIONAL  }
     *                             -- contains a value of the type
     *                             -- registered for use with the
     *                             -- algorithm object identifier value
     * </pre>
     *
     * <p>The algorithm name is determined from the <code>algorithm</code>
     * OID string.
     *
     * @return the signature algorithm name.
     */
    virtual string_view signature_algorithm_name() const = 0;

    /**
     * Gets the public key algorithm name.
     *
     * @return the public key algorithm name.
     */
    virtual string_view key_algorithm_name() const = 0;

    /**
     * Returns the subject (subject distinguished name) value from the
     * certificate.
     *
     * @return an <code>string_view</code> representing the subject
     *		distinguished name
     */
    virtual string_view subject_DN() const = 0;

    /**
     * Returns components of the subject (subject distinguished name)
     * value from the certificate.
     *
     * @return an <code>std::map</code> containing the subject
     *		distinguished name components
     */
    virtual const std::map<string_view, string_view, std::less<>>& subject_DN_components() const = 0;

    /**
     * Returns the issuer (subject distinguished name) value from the
     * certificate.
     *
     * @return an <code>string_view</code> representing the issuer
     *		distinguished name
     */
    virtual string_view issuer_DN() const = 0;

    /**
     * Returns components of the issuer (subject distinguished name)
     * value from the certificate.
     *
     * @return an <code>std::map</code> containing the issuer
     *		distinguished name components
     */
    virtual const std::map<string_view, string_view, std::less<>>& issuer_DN_components() const = 0;

    /**
     * Gets an immutable collection of subject alternative names from the
     * <code>SubjectAltName</code> extension, (OID = 2.5.29.17).
     * <p>
     * The ASN.1 definition of the <code>SubjectAltName</code> extension is:
     * <pre>
     * SubjectAltName ::= GeneralNames
     *
     * GeneralNames :: = SEQUENCE SIZE (1..MAX) OF GeneralName
     *
     * GeneralName ::= CHOICE {
     *      otherName                       [0]     OtherName,
     *      rfc822Name                      [1]     IA5String,
     *      dNSName                         [2]     IA5String,
     *      x400Address                     [3]     ORAddress,
     *      directoryName                   [4]     Name,
     *      ediPartyName                    [5]     EDIPartyName,
     *      uniformResourceIdentifier       [6]     IA5String,
     *      iPAddress                       [7]     OCTET STRING,
     *      registeredID                    [8]     OBJECT IDENTIFIER}
     * </pre>
     * <p>
     * If this certificate does not contain a <code>SubjectAltName</code>
     * extension, <code>null</code> is returned. Otherwise, a
     * <code>Collection</code> is returned with an entry representing each
     * <code>GeneralName</code> included in the extension. Each entry is a
     * <code>List</code> whose first entry is an <code>Integer</code>
     * (the name type, 0-8) and whose second entry is a <code>String</code>
     * or a byte array (the name, in string or ASN.1 DER encoded form,
     * respectively).
     * <p>
     * RFC 822, DNS, and URI names are returned as <code>String</code>s,
     * using the well-established string formats for those types (subject to
     * the restrictions included in RFC 2459). IPv4 address names are
     * returned using dotted quad notation. IPv6 address names are returned
     * in the form "a1:a2:...:a8", where a1-a8 are hexadecimal values
     * representing the eight 16-bit pieces of the address. OID names are
     * returned as <code>String</code>s represented as a series of nonnegative
     * integers separated by periods. And directory names (distinguished names)
     * are returned in RFC 2253 string format. No standard string format is
     * defined for otherNames, X.400 names, EDI party names, or any
     * other type of names. They are returned as byte arrays
     * containing the ASN.1 DER encoded form of the name.
     * <p>
     * Note that the <code>Collection</code> returned may contain more
     * than one name of the same type. Also, note that the returned
     * <code>Collection</code> is immutable and any entries containing byte
     * arrays are cloned to protect against subsequent modifications.
     * <p>
     * This method was added to version 1.4 of the Java 2 Platform Standard
     * Edition. In order to maintain backwards compatibility with existing
     * service providers, this method is not <code>abstract</code>
     * and it provides a default implementation. Subclasses
     * should override this method with a correct implementation.
     *
     * @return an const <code>std::map</code> of subject alternative names
     */
    virtual const std::map<string_view, std::vector<string_view>, std::less<>>& subject_alternative_names() const = 0;

    /**
     * Serial number and issuer of the certificate.
     *
     * <p>The format matches that of the CertificateExactAssertion in
     * RFC4523</p>
     *
     * @return serial number and issuer of the certificate
     */
    virtual string_view certificate_exact_assertion() const = 0;

    /**
     * Returns PEM (Privacy Enhanced Mail) encoded certificates in client
     * certificate chain.
     *
     * @return PEM encoded certificate string
     */
    virtual const std::vector<string_view>& certificate_chain() const = 0;

    /**
     * Returns PEM (Privacy Enhanced Mail) encoded certificate string.
     *
     * @return PEM encoded certificate string
     */
    virtual string_view PEM_encoded() const = 0;
};

/**
 * Enumeration to represent the current state of SSL session.
 */
enum class SSL_SESSION_STATE
{
    /**
     * Represents Initial state of SSL session.
     */
    INITIAL,
    /**
     * Represents Resumed state of SSL session.
     */
    RESUMED
};

/**
 * Output streaming operator overload for SSL_SESSION_STATE enumeration
 *
 * @tparam CharT <code>std::basic_ostream</code> character type.
 * @tparam Traits <code>std::basic_ostream</code> character traits type.
 * @param out output stream
 * @param ss session state
 */
template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, SSL_SESSION_STATE ss)
{
    switch (ss)
    {
        case SSL_SESSION_STATE::INITIAL: out << "INITIAL"; break;
        case SSL_SESSION_STATE::RESUMED: out << "RESUMED"; break;
    }
    return out;
}

/**
 * Class encapsulates availble information about current SSL session.
 *
 * If the current request was not made using SSL channel than this object
 * cannot be accessed from servlet.
 *
 * Note that in order for this information to available in <code>mod_servlet</code>
 * Apache <code>mod_ssl</code> should be configured to enable environment
 * variables. For example:
 *
 * <pre>
 * SSLOptions +StdEnvVars +ExportCertData
 * </pre>
 *
 * For more information on configuration of Apache <code>mod_ssl</code> see
 * <a href="http://httpd.apache.org/docs/current/mod/mod_ssl.html">"Apache
 * Module mod_ssl" documentation</a>
 */
class SSL_information
{
public:
    /**
     * Destructor
     */
    virtual ~SSL_information() noexcept {}

    /**
     * The SSL protocol version (e.q. SSLv3, TLSv1, TLSv1.1, TLSv1.2)
     * @return the SSL protocol version
     */
    virtual string_view protocol() const = 0;

    /**
     * The cipher specification name (e.q. ECDHE-RSA-AES128-GCM-SHA256)
     * @return The cipher specification name
     */
    virtual string_view cipher_name() const = 0;

    /**
     * Returns <code>true</code> if the cipher is an export cipher
     * @return <code>true</code> if the cipher is an export cipher
     */
    virtual bool is_cipher_export() const = 0;

    /**
     * Returns number of bits used in the cipher.
     * @return number of bits used in the cipher
     * @see #cipher_possible_bits
     */
    virtual int cipher_used_bits() const = 0;

    /**
     * Returns number of bits which could possible be used
     * in the cipher.
     *
     * @return possible number of bits to be used in the cipher
     * @see #cipher_used_bits
     */
    virtual int cipher_possible_bits() const = 0;

    /**
     * Returns SSL compression method negotiated.
     * @return SSL compression method negotiated.
     */
    virtual string_view compress_method() const = 0;

    /**
     * The hex-encoded SSL session id if any available.
     * @return The hex-encoded SSL session id
     */
    virtual string_view session_id() const = 0;

    /**
     * Returns the state of the SSL session.
     *
     * <p>It can be eithee <code>INITIAL</code> or <code>RESUMED</code></p>
     * @return the state of the SSL session
     */
    virtual SSL_SESSION_STATE session_state() const = 0;

    /**
     * Returns object containing available information for client
     * certificate
     *
     * @return client certificate information
     */
    virtual const certificate& client_certificate() const = 0;

    /**
     * Returns object containing available information for server
     * certificate
     *
     * @return server certificate information
     */
    virtual const certificate& server_certificate() const = 0;
};

} // end of servlet namespace

#endif // MOD_SERVLET_SSL_H
