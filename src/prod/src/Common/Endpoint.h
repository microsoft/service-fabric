// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <ws2ipdef.h>
#include <mstcpip.h>

//#ifndef _PREFAST_
//#  pragma warning(disable:4068)
//#endif
//#pragma prefast(disable: __WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC, "Endpoint can support both IPv4 and IPv6")

namespace Common
{
    namespace SocketType {
        enum Enum {
            Udp = 0,
            Tcp = 1
        };
    };

    namespace AddressFamily {
        enum Enum {
            Unknown             = -1,   // Unknown
            Unspecified = AF_UNSPEC,    // unspecified
            InterNetwork        = AF_INET,
            InterNetworkV6        = AF_INET6
        };
    };

    //public enum SocketOptionLevel {
    //    Socket = SOL_SOCKET,
    //    IP = ProtocolType.IP,
    //    IPv6 = ProtocolType.IPv6,
    //    Tcp = ProtocolType.Tcp,
    //    Udp = ProtocolType.Udp,

    //}; // enum SocketOptionLevel

    namespace SocketShutdown {
        enum Enum {
            Receive = SD_RECEIVE,
            Send = SD_SEND,
            Both = SD_BOTH,
            None = -1,
        };
    };

    // ISSUE-daviddio-2004/12/7: Static IP address parsing routines that will 
    // hopefully be exposed by NetIO. In fact, first two stolen from old mstcpip.h.
    // #define CXL_IN6_IS_ADDR_ISATAP(a) ((bool)(((a->s6_words[4] & 0xfffd) == 0x0000) && (a->s6_words[5] == 0xfe5e)))
    // #define CXL_IN6_IS_ADDR_6TO4(a) ((bool)(a->s6_words[0] == 0x0220))

    int32 CreateMask(int bitsToSet);

    class Socket;

    class Endpoint
    {
    public:
        __declspec(property(get=get_AddressFamily)) AddressFamily::Enum AddressFamily;
        __declspec(property(get=get_AsSockAddr))    ::sockaddr*         AsSockAddr;
        __declspec(property(get=get_AddressLength)) int                 AddressLength;
        __declspec(property(get=get_Port,put=put_Port)) ::USHORT        Port;
        __declspec(property(get=get_ScopeId,put=put_ScopeId)) ULONG ScopeId;

    public:

        static ErrorCode TryParse(std::wstring const & input, Endpoint & output);

        explicit Endpoint() 
        {
            ZeroMemory( &address, sizeof(address) );
        }

        explicit Endpoint( std::wstring const & address, int port = 0 );

        explicit Endpoint ( ::ADDRINFOW const & addrInfo );

        explicit Endpoint ( ::sockaddr const & addr );

        explicit Endpoint ( SOCKET_ADDRESS const & address );

        explicit Endpoint ( int32 networkByteOrderAddress );

        explicit Endpoint(Socket const & socket);

        static ErrorCode GetSockName(Socket const & socket, _Out_ Endpoint & localEndpoint);

        void ToString( std::wstring & result) const;

        std::wstring ToString() const;

        const ::sockaddr* get_AsSockAddr() const 
        {
            return reinterpret_cast<const sockaddr*>(&this->address);
        }

        int get_AddressLength() const
        {
            debug_assert( address.ss_family == AF_INET6 || address.ss_family == AF_INET );

#pragma prefast(suppress: 24002, "IPv4 and IPv6 code paths provided")
            return ( address.ss_family == AF_INET ) ? sizeof( sockaddr_in ) : sizeof ( sockaddr_in6 );
        }

        ULONG get_ScopeId() const
        {
            ASSERT_IFNOT(address.ss_family == AF_INET6, "scoped id is supported only for IPv6 address");
            return reinterpret_cast<const SOCKADDR_IN6*>(&address)->sin6_scope_id;
        }

        //
        // Do not complain if the family is not AF_INET6
        //
        void put_ScopeId(ULONG scopeId)
        {
            if ( address.ss_family == AF_INET6 ) {
                reinterpret_cast<SOCKADDR_IN6*>(&address)->sin6_scope_id = scopeId;
            }
        }

        ::USHORT get_Port() const
        {
            // note that this works also for sockaddr_in6
            return ntohs(reinterpret_cast<const struct ::sockaddr_in*>(&address)->sin_port);
        }

        void put_Port( ::USHORT value )
        {
            // note that this works also for sockaddr_in6
            reinterpret_cast<struct ::sockaddr_in*>(&address)->sin_port = htons(value);
        }

        AddressFamily::Enum get_AddressFamily() const
        {
            return AddressFamily::Enum(address.ss_family);
        }

        //
        // IsLinkLocal and IsSiteLocal are defined in
        // topologymanager/IPAddressPrefix.cpp
        //
        bool IsLinkLocal() const;

        bool IsSiteLocal() const;

        bool IsIPV4() const
        { 
            return (address.ss_family == AF_INET);
        }

        bool IsIPV6() const
        { 
            return (address.ss_family == AF_INET6);
        }

        bool IsIPv4 () const
        {
            return IsIPV4();
        }

        bool IsIPv6 () const
        {
            return IsIPV6();
        }

        // ipString must be large enough to hold INET_ADDRSTRLEN/INET6_ADDRSTRLEN characters, depending on address type
        void GetIpString(_Out_writes_(size) WCHAR * ipString, socklen_t size = INET6_ADDRSTRLEN) const;
        void GetIpString(_Out_ std::wstring & ipString) const;
        std::wstring GetIpString() const;
        std::wstring GetIpString2() const; // [] enclosure is added if IPv6

        bool IsLoopback() const;
        SCOPE_LEVEL IpScopeLevel() const;

        static bool IsEndpointIPv6(Common::Endpoint const & e)
        {
            return e.IsIPv6();
        }

        bool Is6to4() const
        {
            if ( !IsIPV6() ) {
                return false;
            }

            return ( IN6ADDR_IS6TO4(this->as_sockaddr_in6()) == TRUE );
        }

        bool IsISATAP() const
        {
            if ( !IsIPV6() ) {
                return false;
            }

            return ( IN6ADDR_ISISATAP(this->as_sockaddr_in6()) == TRUE );
        }

    public:
        struct ::sockaddr_storage * as_sockaddr_storage ()
        {
            return ( &address );
        }

        struct ::sockaddr_storage const * as_sockaddr_storage () const
        {
            return reinterpret_cast<sockaddr_storage const *>( &address );
        }

        struct ::sockaddr * as_sockaddr () 
        {
            return reinterpret_cast<sockaddr*>( &address );
        }

        struct ::sockaddr const * as_sockaddr () const
        {
            return reinterpret_cast<sockaddr const *>( &address );
        }

        struct ::sockaddr_in * as_sockaddr_in ()
        {
            return reinterpret_cast<sockaddr_in*>( &address );
        }

        struct ::sockaddr_in const * as_sockaddr_in () const
        {
            return reinterpret_cast<sockaddr_in const *>( &address );
        }

        struct ::sockaddr_in6 * as_sockaddr_in6 ()
        {
            return reinterpret_cast<sockaddr_in6*>( &address );
        }
        
        struct ::sockaddr_in6 const * as_sockaddr_in6 () const
        {
            return reinterpret_cast<sockaddr_in6 const *>( &address );
        }

        operator const ::sockaddr* () const
        {
            return as_sockaddr();
        }

        operator ::sockaddr* ()
        {
            return as_sockaddr();
        }

        //
        // Equality operators
        //
        bool EqualPort ( Endpoint const & rhs ) const
        {
            return ( this->Port == rhs.Port );
        }

        std::wstring AsSmbServerName() const;

        //
        // Compare without the port
        //
        bool EqualAddress ( Endpoint const & rhs ) const;

        //
        // Both address and port must match
        //
        bool operator == ( Endpoint const & rhs) const 
        {
            return ( EqualAddress( rhs ) && EqualPort( rhs ) );
        }

        //
        // No port comparison. 
        //
        bool EqualPrefix ( Endpoint const & rhs, int prefixLength ) const;
        
        //
        // operator <
        // Address family must match (else AF_INET prefered over AF_IET6)
        // Else simple < of sin_arr or sin6_addr
        //
        bool operator < (Endpoint const &rhs) const;

        //
        // Operator < on a prefix of the address
        // Port is ignored
        //
        bool LessPrefix ( Endpoint const & rhs, int const prefixLength ) const;
        
        //
        // Write To
        //
        void Endpoint::WriteTo ( TextWriter & w, FormatOptions const & ) const;

    private:
        ::sockaddr_storage address;
    };

    //template <> struct SerializerTrait<Endpoint>
    //{
    //    static void ReadFrom(SerializeReader & r, Endpoint & value, std::wstring const & name);
    //    static void WriteTo(SerializeWriter & w, Endpoint const & value, std::wstring const & name);
    //};

    //typedef std::pair< Endpoint, Endpoint > EndpointPair;

    extern const Endpoint IPv4AnyAddress;
    extern const Endpoint IPv6AnyAddress;

    class EndpointHash
    {
    public:
        size_t operator()(Endpoint const & endpoint) const;
    };
};
