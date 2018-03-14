// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if defined(PLATFORM_UNIX)

typedef int DNS_STATUS;
typedef  int DNS_QUERY_RESULT;

#define DNS_ERROR_RCODE_FORMAT_ERROR     9001L

#define DNS_ERROR_RCODE_SERVER_FAILURE   9002L

#define DNS_ERROR_RCODE_NAME_ERROR       9003L

#define DNS_INFO_NO_RECORDS              9501L


//
//  DNS Record Types
//
//  _TYPE_ defines are in host byte order.
//  _RTYPE_ defines are in net byte order.
//
//  Generally always deal with types in host byte order as we index
//  resource record functions by type.
//

#define DNS_TYPE_ZERO       0x0000

//  RFC 1034/1035
#define DNS_TYPE_A          0x0001      //  1
#define DNS_TYPE_NS         0x0002      //  2
#define DNS_TYPE_MD         0x0003      //  3
#define DNS_TYPE_MF         0x0004      //  4
#define DNS_TYPE_CNAME      0x0005      //  5
#define DNS_TYPE_SOA        0x0006      //  6
#define DNS_TYPE_MB         0x0007      //  7
#define DNS_TYPE_MG         0x0008      //  8
#define DNS_TYPE_MR         0x0009      //  9
#define DNS_TYPE_NULL       0x000a      //  10
#define DNS_TYPE_WKS        0x000b      //  11
#define DNS_TYPE_PTR        0x000c      //  12
#define DNS_TYPE_HINFO      0x000d      //  13
#define DNS_TYPE_MINFO      0x000e      //  14
#define DNS_TYPE_MX         0x000f      //  15
#define DNS_TYPE_TEXT       0x0010      //  16

//  RFC 1183
#define DNS_TYPE_RP         0x0011      //  17
#define DNS_TYPE_AFSDB      0x0012      //  18
#define DNS_TYPE_X25        0x0013      //  19
#define DNS_TYPE_ISDN       0x0014      //  20
#define DNS_TYPE_RT         0x0015      //  21

//  RFC 1348
#define DNS_TYPE_NSAP       0x0016      //  22
#define DNS_TYPE_NSAPPTR    0x0017      //  23

//  RFC 2065    (DNS security)
#define DNS_TYPE_SIG        0x0018      //  24
#define DNS_TYPE_KEY        0x0019      //  25

//  RFC 1664    (X.400 mail)
#define DNS_TYPE_PX         0x001a      //  26

//  RFC 1712    (Geographic position)
#define DNS_TYPE_GPOS       0x001b      //  27

//  RFC 1886    (IPv6 Address)
#define DNS_TYPE_AAAA       0x001c      //  28

//  RFC 1876    (Geographic location)
#define DNS_TYPE_LOC        0x001d      //  29

//  RFC 2065    (Secure negative response)
#define DNS_TYPE_NXT        0x001e      //  30

//  Patton      (Endpoint Identifier)
#define DNS_TYPE_EID        0x001f      //  31

//  Patton      (Nimrod Locator)
#define DNS_TYPE_NIMLOC     0x0020      //  32

//  RFC 2052    (Service location)
#define DNS_TYPE_SRV        0x0021      //  33

//  ATM Standard something-or-another (ATM Address)
#define DNS_TYPE_ATMA       0x0022      //  34

//  RFC 2168    (Naming Authority Pointer)
#define DNS_TYPE_NAPTR      0x0023      //  35

//  RFC 2230    (Key Exchanger)
#define DNS_TYPE_KX         0x0024      //  36

//  RFC 2538    (CERT)
#define DNS_TYPE_CERT       0x0025      //  37

//  A6 Draft    (A6)
#define DNS_TYPE_A6         0x0026      //  38

//  DNAME Draft (DNAME)
#define DNS_TYPE_DNAME      0x0027      //  39

//  Eastlake    (Kitchen Sink)
#define DNS_TYPE_SINK       0x0028      //  40

//  RFC 2671    (EDNS OPT)
#define DNS_TYPE_OPT        0x0029      //  41

//  RFC 4034    (DNSSEC DS)
#define DNS_TYPE_DS         0x002b      //  43

//  RFC 4034    (DNSSEC RRSIG)
#define DNS_TYPE_RRSIG      0x002e      //  46

//  RFC 4034    (DNSSEC NSEC)
#define DNS_TYPE_NSEC       0x002f      //  47

//  RFC 4034    (DNSSEC DNSKEY)
#define DNS_TYPE_DNSKEY     0x0030      //  48

//  RFC 4701    (DHCID)
#define DNS_TYPE_DHCID      0x0031      //  49

//  RFC 5155    (DNSSEC NSEC3)
#define DNS_TYPE_NSEC3      0x0032      //  50

//  RFC 5155    (DNSSEC NSEC3PARAM)
#define DNS_TYPE_NSEC3PARAM 0x0033      //  51

//RFC 6698	(TLSA)
#define DNS_TYPE_TLSA	    0x0034      //  52

//
//  IANA Reserved
//

#define DNS_TYPE_UINFO      0x0064      //  100
#define DNS_TYPE_UID        0x0065      //  101
#define DNS_TYPE_GID        0x0066      //  102
#define DNS_TYPE_UNSPEC     0x0067      //  103

//
//  Query only types (1035, 1995)
//      - Crawford      (ADDRS)
//      - TKEY draft    (TKEY)
//      - TSIG draft    (TSIG)
//      - RFC 1995      (IXFR)
//      - RFC 1035      (AXFR up)
//

#define DNS_TYPE_ADDRS      0x00f8      //  248
#define DNS_TYPE_TKEY       0x00f9      //  249
#define DNS_TYPE_TSIG       0x00fa      //  250
#define DNS_TYPE_IXFR       0x00fb      //  251
#define DNS_TYPE_AXFR       0x00fc      //  252
#define DNS_TYPE_MAILB      0x00fd      //  253
#define DNS_TYPE_MAILA      0x00fe      //  254
#define DNS_TYPE_ALL        0x00ff      //  255
#define DNS_TYPE_ANY        0x00ff      //  255

#endif


#if !defined(PLATFORM_UNIX)
#define STRPRINT(buffer, buffersize, format, ...) \
_snwprintf_s(buffer, buffersize, format, __VA_ARGS__)
#else
#define STRPRINT(buffer, buffersize, format, ...) \
swprintf(buffer, buffersize, format, __VA_ARGS__)
#endif

namespace DNS { namespace Test
{
    struct DNS_DATA
    {
        WORD Type;
        KString::SPtr DataStr;
    };

    namespace DnsHelper
    {
        void CreateDnsServiceHelper(
            __in KAllocator& allocator,
            __in DnsServiceParams& params,
            __out IDnsService::SPtr& spService,
            __out ComServiceManager::SPtr& spServiceManager,
            __out ComPropertyManager::SPtr& spPropertyManager,
            __out FabricData::SPtr& spData,
            __out IDnsParser::SPtr& spParser,
            __out INetIoManager::SPtr& spNetIoManager
        );

        DNS_STATUS Query(
            __in INetIoManager& netIoManager,
            __in KBuffer& buffer,
            __in IDnsParser& dnsParser,
            __in USHORT port,
            __in LPCWSTR wszQuery,
            __in USHORT type,
            __in KArray<DNS_DATA>& results
        );

        DNS_STATUS QueryEx(
            __in KBuffer& buffer,
            __in LPCWSTR wszQuery,
            __in USHORT type,
            __in KArray<DNS_DATA>& results
        );

        void SerializeQuestion(
            __in IDnsParser& dnsParser,
            __in LPCWSTR wszQuery,
            __in KBuffer& buffer,
            __in WORD type,
            __out ULONG& size,
	        __in DnsTextEncodingType encodingType = DnsTextEncodingTypeUTF8
        );
    };
}}
