// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#ifdef PLATFORM_UNIX

#include <arpa/inet.h>

typedef void* LPWSAPROTOCOL_INFOW;

static INT WSAAPI WSAStringToAddressW(
    LPWSTR	AddressString,
    INT		AddressFamily,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPSOCKADDR 	lpAddress,
    LPINT	lpAddressLength
    )
{
    memset(lpAddress, *lpAddressLength, 0);
    std::string addrStr;
    Common::StringUtility::Utf16ToUtf8(AddressString, addrStr);

    void * ipPtr = nullptr;
    int outputSize = *lpAddressLength;
    if (AddressFamily == AF_INET)
    {
        if (*lpAddressLength < sizeof(sockaddr_in))
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        outputSize = sizeof(sockaddr_in);
        ipPtr = &(((sockaddr_in*)lpAddress)->sin_addr);
    }
    else if(AddressFamily == AF_INET6)
    {
        if (*lpAddressLength < sizeof(sockaddr_in6))
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        outputSize = sizeof(sockaddr_in6);
        ipPtr = &(((sockaddr_in6*)lpAddress)->sin6_addr);
    }
    else
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    memset(lpAddress, *lpAddressLength, 0);
    if(inet_pton(AddressFamily, addrStr.c_str(), ipPtr) == 1)
    {
        lpAddress->sa_family = AddressFamily;
        *lpAddressLength = outputSize;
        return 0;
    }

    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

#endif

namespace Common 
{
    using namespace std;

    // 
    // Returns a 32 bit mask to be used with adresses in network byte order
    //
    int32 CreateMask(int bitsToSet)
    {
        int32 result = ((bitsToSet != 0) ? ~((1 << (32-bitsToSet)) - 1) : 0);
        return htonl(result);
    }

    size_t EndpointHash::operator()(Endpoint const & endpoint) const
    {
        auto sockAddrStorage = endpoint.as_sockaddr_storage();
        if (sockAddrStorage->ss_family == AF_INET)
        {
            auto sockAddrIn = (::sockaddr_in const*)sockAddrStorage;
            return std::hash<ULONG>()((ULONG&)(sockAddrIn->sin_addr)) ^
                std::hash<ULONG>()((ULONG)(sockAddrIn->sin_port));
        }

        if (sockAddrStorage->ss_family == AF_INET6)
        {
            auto sockAddrIn6 = (::sockaddr_in6 const*)sockAddrStorage;
            auto addrIn6 = &(sockAddrIn6->sin6_addr);
            UINT64 const * addrIn6AsULonglong = (UINT64 const *)addrIn6;
            return
                std::hash<ULONG>()((ULONG)(sockAddrIn6->sin6_port)) ^
                std::hash<UINT64>()(addrIn6AsULonglong[0]) ^
                std::hash<UINT64>()(addrIn6AsULonglong[1]);
        }

        UINT64 const * asULonglongPtr = (UINT64 const *)sockAddrStorage;
        uint count = sizeof(sockaddr_storage) / sizeof(UINT64);
        size_t result = 0;
        for (uint i = 0; i < count; ++i)
        {
            result ^= std::hash<UINT64>()(asULonglongPtr[i]);
        }

        return result;
    }

    ErrorCode Endpoint::TryParse(std::wstring const & str, Endpoint & ep)
    {
        ::ZeroMemory(&ep.address, sizeof(ep.address));

        int size = sizeof( ep.address );

        //
        // Try to first convert to an IPv4 address
        //
        int wsaError = ::WSAStringToAddressW( const_cast<LPWSTR>(str.c_str()),
            AF_INET, 
            nullptr, 
            ep.as_sockaddr(),
            &size );

        if ( (wsaError == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL) ) 
        {
            //
            // If that fails, try to convert to an IPv6 address
            //
            wsaError = ::WSAStringToAddressW(
                const_cast<LPWSTR>(str.c_str()), 
                AF_INET6,
                nullptr,
                ep.as_sockaddr(),
                &size );
        }

        if (0 != wsaError)
        {
            SetLastError(WSAGetLastError());
            return ErrorCode::FromWin32Error();
        }

        return ErrorCodeValue::Success;
    }


    //////////////////////////////////////////////////////////////////////
    //
    // Endpoint
    //
    //////////////////////////////////////////////////////////////////////
    Endpoint::Endpoint (
        std::wstring const & str,
        int             port
        )
    {
        if ( !TryParse(str, *this).IsSuccess() )
            THROW(WinError(GetLastError()), "failed to parse string as ip address");

        //
        // Set the port
        //
        argument_in_range(port, 0, 0xFFFF);
        Port = static_cast<::u_short>( port );
    }

    Endpoint::Endpoint ( ::ADDRINFOW const & addrInfo )
    {
        ::ZeroMemory(&address, sizeof(address));

        ASSERT_IFNOT(
            addrInfo.ai_addrlen <= sizeof(address),
            "addrInfo.ai_addrlen <= sizeof(address)");
        KMemCpySafe(&address, sizeof(address), addrInfo.ai_addr, addrInfo.ai_addrlen);

        ASSERT_IFNOT(
            get_AddressFamily() == AddressFamily::InterNetwork || get_AddressFamily() == AddressFamily::InterNetworkV6,
            "Invalid address family");
    }

    Endpoint::Endpoint ( ::sockaddr const & addr )
    {
        //
        // It is acceptable for sockaddr to be completely 0
        //
        ASSERT_IFNOT(
            addr.sa_family == AF_INET || addr.sa_family == AF_INET6 || addr.sa_family == 0,
            "Invalid address family");

#pragma prefast(suppress: 24002, "IPv4 and IPv6 code paths provided")
        size_t size = ( addr.sa_family == AF_INET ) ? sizeof( sockaddr_in ) : sizeof( sockaddr_in6 );
        ::ZeroMemory( &address, sizeof( address ) );
        KMemCpySafe(&address, sizeof(address), &addr, size);
    }

    Endpoint::Endpoint ( ::SOCKET_ADDRESS const & addr ) 
    {
        ASSERT_IFNOT(
            addr.lpSockaddr->sa_family == AF_INET || addr.lpSockaddr->sa_family == AF_INET6,
            "Invalid address family");

#pragma prefast(suppress: 24002, "IPv4 and IPv6 code paths provided")
        size_t size = ( addr.lpSockaddr->sa_family == AF_INET ) ? sizeof( sockaddr_in ) : sizeof( sockaddr_in6 );
        ::ZeroMemory( &address, sizeof( address ) );
        KMemCpySafe(&address, sizeof(address), addr.lpSockaddr, size);
    }

    Endpoint::Endpoint ( int32 networkByteOrderAddress )
    {
        ::ZeroMemory( &address, sizeof( address ) );
        KMemCpySafe(
            &(reinterpret_cast<struct ::sockaddr_in*>(&address)->sin_addr),
            sizeof(reinterpret_cast<struct ::sockaddr_in*>(&address)->sin_addr),
            &networkByteOrderAddress, 
            sizeof(networkByteOrderAddress));
        address.ss_family = (short)AddressFamily::InterNetwork;
    }

    Endpoint::Endpoint(Socket const & socket)
    {
        ::ZeroMemory( &address, sizeof(address) );

        socklen_t size = static_cast<socklen_t>(sizeof(address));
        CHK_WSA( ::getsockname(socket.GetHandle() ,
            reinterpret_cast<LPSOCKADDR>( &this->address ),
            &size ) );
    }

    _Use_decl_annotations_ ErrorCode Endpoint::GetSockName(Socket const & socket, Endpoint & localEndpoint)
    {
        socklen_t size = static_cast<socklen_t>(sizeof(localEndpoint.address));
        if (::getsockname(socket.GetHandle(), (LPSOCKADDR)(&localEndpoint.address), &size) != 0)
        {
            return ErrorCode::FromWin32Error();
        }

        return ErrorCode();
    }

    bool Endpoint::IsLoopback()const
    {
        if (IsIPv4())
        {
            static const int loopbackMask = INADDR_LOOPBACK & 0xff000000;
            auto thisMask = ntohl(as_sockaddr_in()->sin_addr.s_addr) & 0xff000000;
            return thisMask == loopbackMask; 
        }

        Invariant(IsIPv6());
        return ::memcmp(&(as_sockaddr_in6()->sin6_addr), &in6addr_loopback, sizeof(in6_addr)) == 0;
    }

    SCOPE_LEVEL Endpoint::IpScopeLevel() const
    {
        if (IsLoopback()) return ScopeLevelInterface;
        if (IsLinkLocal()) return ScopeLevelLink;
        if (IsSiteLocal()) return ScopeLevelSite;

        return ScopeLevelGlobal;
    }

    _Use_decl_annotations_ void Endpoint::GetIpString(WCHAR * ipString, socklen_t size) const
    {
#ifdef PLATFORM_UNIX
        wstring buffer;
        GetIpString(buffer);
        memcpy(ipString, buffer.c_str(), (buffer.size() + 1) * sizeof(WCHAR));
#else
        if (IsIPv4())
        {
            Invariant(size >= INET_ADDRSTRLEN);
            RtlIpv4AddressToString(&as_sockaddr_in()->sin_addr, ipString);
            return;
        }

        Invariant(IsIPv6());
        Invariant(size >= INET6_ADDRSTRLEN);
        RtlIpv6AddressToString(&as_sockaddr_in6()->sin6_addr, ipString);
#endif
    }

    _Use_decl_annotations_ void Endpoint::GetIpString(std::wstring & ipString) const
    {
#ifdef PLATFORM_UNIX
        char buffer[INET6_ADDRSTRLEN];
        if (IsIPv4())
        {
            Invariant(inet_ntop(AF_INET, &as_sockaddr_in()->sin_addr, buffer, sizeof(buffer)));
        }
        else
        {
            Invariant(IsIPv6());
            Invariant(inet_ntop(AF_INET6, &as_sockaddr_in6()->sin6_addr, buffer, sizeof(buffer)));
        }

        StringUtility::Utf8ToUtf16(string(buffer), ipString);
#else 
        WCHAR buffer[INET6_ADDRSTRLEN];
        GetIpString(buffer);
        ipString = std::wstring(buffer);
#endif
    }

    std::wstring Endpoint::GetIpString() const
    {
        std::wstring ipString;
        GetIpString(ipString);
        return ipString;
    }

    std::wstring Endpoint::GetIpString2() const
    {
        if (IsIPv4())
        {
            return GetIpString();
        }

        Invariant(IsIPv6());
        return wformatString("[{0}]", GetIpString());
    }

    std::wstring Endpoint::ToString() const
    {
        std::wstring buffer;
        this->ToString(buffer);
        return buffer;
    }

    void Endpoint::ToString(std::wstring& result) const
    {
        result = wformatString("{0}:{1}", GetIpString2(), Port);
    }

    std::wstring Endpoint::AsSmbServerName() const
    {
        return wformatString("\\\\{0}", GetIpString2());
    }

    // Note: Code in clussvc that compares endpoints relies on the current EqualAddress
    // implementation where the scope id is ignored. Remote IPv6 link local addresses in clussvc
    // have their scope ids stripped off before they are passed to netft, however, it is still
    // expected they will pass the EqualAddress check when compared to the original value.
    bool Endpoint::EqualAddress ( Endpoint const & rhs ) const
    {
        if ( address.ss_family != rhs.address.ss_family ) 
        {
            return false;
        }

        if ( address.ss_family == AF_INET ) 
        {
            return 0 == ::memcmp( 
                &reinterpret_cast<const struct ::sockaddr_in*>(&address)->sin_addr,
                &reinterpret_cast<const struct ::sockaddr_in*>(&rhs.address)->sin_addr,
                sizeof(in_addr));
        }
        else if ( address.ss_family == AF_INET6 )
        {
            return 0 == ::memcmp(
                &reinterpret_cast<const SOCKADDR_IN6*>(&address)->sin6_addr,
                &reinterpret_cast<const SOCKADDR_IN6*>(&rhs.address)->sin6_addr,
                sizeof( in6_addr ) );
        }
        else
        {
            // unknown address family
            return 0 == ::memcmp( &address,
                &rhs.address,
                sizeof( address ) );
        }
    }

    //
    // This is a seperate implementation because it is much more efficient
    // to directly compare the addresses rather than prefixes
    //
    bool Endpoint::EqualPrefix ( Endpoint const & rhs, int prefixLength ) const
    {

        if ( address.ss_family != rhs.address.ss_family ) 
        {
            return false;
        }

        if ( address.ss_family == AF_INET ) 
        {

            argument_in_range( prefixLength, 0, 32 );
            
            //
            // IPv4 is easy to compare because the mask is a 32 bit int
            //
            unsigned int mask = CreateMask( prefixLength );    
            return ( ( mask & ((ULONG &)(as_sockaddr_in()->sin_addr))) ==
                ( mask & ((ULONG &)(rhs.as_sockaddr_in()->sin_addr))));

        }
        else
        {

            argument_in_range( prefixLength, 0, 128 );

            //
            // for IPv6 we compare the bytes with a full prefix first
            //
            int comparableBytes = prefixLength / 8;
            if ( ::memcmp ( & as_sockaddr_in6()->sin6_addr,
                & rhs.as_sockaddr_in6()->sin6_addr,
                comparableBytes ) != 0 ) 
            {
                    return false;
            }

            //
            // if prefixLength is a multiple of 8 we're done
            //
            if ( prefixLength % 8 == 0 )
                return true;
            
            //
            // Next compare the last byte with the half prefix
            //
            byte mask = 0xff << ( 8 - static_cast<byte>( prefixLength % 8 ) );
            byte const * lhsSin6Addr = (byte const *)(&as_sockaddr_in6()->sin6_addr);
            byte const * rhsSin6Addr = (byte const *)(&rhs.as_sockaddr_in6()->sin6_addr);

            //
            // Try not to cause a buffer overflow
            //
            ASSERT_IFNOT(comparableBytes != 16, "comparableBytes != 16");

            return ( ( mask & lhsSin6Addr[comparableBytes] ) ==
                ( mask & rhsSin6Addr[comparableBytes] ) );
        }
    }

    bool Endpoint::operator < (Endpoint const &rhs) const
    {
        if (address.ss_family < rhs.address.ss_family) 
        {
            return true;
        }
        else if (rhs.address.ss_family < address.ss_family)
        {
            return false;
        }
        else
        {
            if (address.ss_family == AF_INET) 
            {
                return 
                    0 < ::memcmp(
                            &reinterpret_cast<const struct ::sockaddr_in*>(&address)->sin_addr,
                            &reinterpret_cast<const struct ::sockaddr_in*>(&rhs.address)->sin_addr,
                            sizeof(in_addr));
            }
            else
            {
                return
                    0 < ::memcmp(
                    &reinterpret_cast<const SOCKADDR_IN6*>(&address)->sin6_addr,
                    &reinterpret_cast<const SOCKADDR_IN6*>(&rhs.address)->sin6_addr,
                    sizeof(reinterpret_cast<const SOCKADDR_IN6*>(&address)->sin6_addr)
                    );
            }
        }
    }

    bool Endpoint::LessPrefix ( Endpoint const & rhs, int const prefixLength ) const
    {
        if ( address.ss_family != rhs.address.ss_family ) 
            return false;

        if ( address.ss_family == AF_INET ) 
        {
            argument_in_range( prefixLength, 0, 32 );

            unsigned int mask = CreateMask( prefixLength );    
            return ( ( mask & ((ULONG&)as_sockaddr_in()->sin_addr)) <
                ( mask & ((ULONG&)rhs.as_sockaddr_in()->sin_addr)) );

        }
        else
        {
            ASSERT_IFNOT(address.ss_family == AF_INET6, "address.ss_family == AF_INET6");
            argument_in_range( prefixLength, 0, 128 );

            //
            // Compare full bytes first
            //
            int comparableBytes = prefixLength / 8;
            int memcmpResult = ::memcmp ( & as_sockaddr_in6()->sin6_addr,
                & rhs.as_sockaddr_in6()->sin6_addr,
                comparableBytes );
            if ( memcmpResult != 0 ) 
            {
                return memcmpResult < 0;
            }

            //
            // if prefixLength is a multiple of 8 we're done
            // The prefixes so far are exaclty the same and so the
            // less fails
            //
            if ( prefixLength % 8 == 0 )
                return false;

            //
            // Next compare the last byte with the half prefix
            //
            byte mask = 0xff << ( 8 - static_cast<byte>( prefixLength % 8 ) );
            byte const * lhsSin6Addr = (byte const *)(&as_sockaddr_in6()->sin6_addr);
            byte const * rhsSin6Addr = (byte const *)(&rhs.as_sockaddr_in6()->sin6_addr);

            //
            // Try not to cause a buffer overflow
            //
            ASSERT_IFNOT(comparableBytes != 16, "comparableBytes != 16");

            return ( ( mask & lhsSin6Addr[comparableBytes] ) <
                ( mask & rhsSin6Addr[comparableBytes] ) );
        }
    }

    void Endpoint::WriteTo ( TextWriter & w, FormatOptions const &) const
    {
        w << ToString();
    }

#if 0
    void SerializerTrait<Endpoint>::ReadFrom(SerializeReader & r, Endpoint & value, std::wstring const & name)
    {
        std::wstring temp;
        r.Read(temp, name);
        std::wstring address, port;

        Split(temp, ' ', address, port);
        value = Endpoint(address, Int32_Parse(port));
    }

    void SerializerTrait<Endpoint>::WriteTo(SerializeWriter & w, Endpoint const & value, std::wstring const & name)
    {
        std::wstring temp;
        value.ToString(temp);
        temp.push_back(' ');
        temp.append( Int32_ToString(value.Port) );
        w.Write(temp, name);
    }
#endif
    //////////////////////////////////////////////////////////////////////
    //
    // Common address definitions
    //
    //////////////////////////////////////////////////////////////////////
    namespace detail 
    {
#pragma prefast(suppress: 24002, "IPv4 and IPv6 code paths provided")
        sockaddr_in const _any_in = { AF_INET,
                                      0,
                                      in4addr_any,
                                      { 0 } };
        sockaddr const * any_in = reinterpret_cast<sockaddr const *>( &_any_in );

        sockaddr_in6 const _any_in6 = { AF_INET6,
                                        0,
                                        0,
                                        in6addr_any,
                                        0 };
        sockaddr const * any_in6 = reinterpret_cast<sockaddr const *>( &_any_in6 );

    }

    const Endpoint IPv4AnyAddress = Endpoint( * detail::any_in );

    const Endpoint IPv6AnyAddress = Endpoint( * detail::any_in6 );
    
};
