// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <iterator>

namespace Common 
{
    in_addr Ipv4PrefixAssembler(UCHAR s0, UCHAR s1 = 0)
    {
        in_addr ipv4Prefix;
        ((UCHAR*)&ipv4Prefix)[0] = s0;
        ((UCHAR*)&ipv4Prefix)[1] = s1;
        return ipv4Prefix;
    }

#pragma prefast(push)
#pragma prefast(disable: 24002, "IPv4 and IPv6 code paths provided")
    namespace prefix 
    {
        namespace detail 
        {
            sockaddr_in const _linklocal_in = { AF_INET,
                                                0,
                                                Ipv4PrefixAssembler(169, 254),
                                                { 0 } };
            sockaddr const * linklocal_in = reinterpret_cast<sockaddr const *>( &_linklocal_in );

            sockaddr_in const _sitelocal_in_1 = { AF_INET,
                                                  0,
                                                  Ipv4PrefixAssembler(10),
                                                  { 0 } };
            sockaddr const * sitelocal_in_1 = reinterpret_cast<sockaddr const *>( &_sitelocal_in_1 );

            sockaddr_in const _sitelocal_in_2 = { AF_INET,
                                                  0,
                                                  Ipv4PrefixAssembler(172, 16),
                                                  { 0 } };
            sockaddr const * sitelocal_in_2 = reinterpret_cast<sockaddr const *>( &_sitelocal_in_2 );

            sockaddr_in const _sitelocal_in_3 = { AF_INET,
                                                  0,
                                                  Ipv4PrefixAssembler(192, 168),
                                                 { 0 } };
            sockaddr const * sitelocal_in_3 = reinterpret_cast<sockaddr const *>( &_sitelocal_in_3 );

            sockaddr_in6 const _linklocal_in6 = { AF_INET6,
                                                  0,
                                                  0,
                                                  in6addr_linklocalprefix,
                                                  0 };
            sockaddr const * linklocal_in6 = reinterpret_cast<sockaddr const *>( &_linklocal_in6 );

            sockaddr_in6 const _sitelocal_in6 = { AF_INET6,
                                                  0,
                                                  0,
                                                  { 0xfe,
                                                    0xc0 },
                                                  0 };
            sockaddr const * sitelocal_in6 = reinterpret_cast<sockaddr const *>( &_sitelocal_in6 );
        };
#pragma prefast(pop)
        
        const IPPrefix IPv4LinkLocal ( Endpoint( *detail::linklocal_in ),
                                       16 );

        const IPPrefix IPv4SiteLocal1 ( Endpoint( *detail::sitelocal_in_1 ),
                                        8 );

        const IPPrefix IPv4SiteLocal2 ( Endpoint( *detail::sitelocal_in_2 ),
                                        12 );
        
        const IPPrefix IPv4SiteLocal3 ( Endpoint( *detail::sitelocal_in_3 ),
                                        16 );

        //
        // There is some confusion about the link local prefix being 10 or 64
        //
        const IPPrefix IPv6LinkLocal ( Endpoint( *detail::linklocal_in6 ),
                                       IN6ADDR_LINKLOCALPREFIX_LENGTH );

        const IPPrefix IPv6SiteLocal ( Endpoint( *detail::sitelocal_in6 ),
                                       10 );
    };
    
    bool Endpoint::IsLinkLocal() const
    {
        ASSERT_IFNOT( IsIPV4() || IsIPV6(), "IsIPV4() || IsIPV6()");
        
        if ( IsIPV4() ) 
        {

            return EqualPrefix( prefix::IPv4LinkLocal.GetAddress(),
                                prefix::IPv4LinkLocal.GetPrefixLength() );

        }
        else
        {

            return TRUE == IN6_IS_ADDR_LINKLOCAL( & as_sockaddr_in6()->sin6_addr );
        }
    }

    bool Endpoint::IsSiteLocal() const
    {
        ASSERT_IFNOT( IsIPV4() || IsIPV6(), "IsIPV4() || IsIPV6()");

        if ( IsIPV4() ) 
        {

            return ( EqualPrefix( prefix::IPv4SiteLocal1.GetAddress(),
                                  prefix::IPv4SiteLocal1.GetPrefixLength() ) ||
                     EqualPrefix( prefix::IPv4SiteLocal2.GetAddress(),
                                  prefix::IPv4SiteLocal2.GetPrefixLength() ) ||
                     EqualPrefix( prefix::IPv4SiteLocal3.GetAddress(),
                                  prefix::IPv4SiteLocal3.GetPrefixLength() ) );

        }
        else
        {

            return TRUE == IN6_IS_ADDR_SITELOCAL( & as_sockaddr_in6()->sin6_addr );
        }
    }

    bool DoesAddressMatchPrefix( Endpoint const & address, IPPrefix const & prefix ) 
    {
        return address.EqualPrefix( prefix.GetAddress(), prefix.GetPrefixLength() );
    }

    bool IPPrefix::operator == ( IPPrefix const & rhs ) const
    { 
        return ( ( prefixLength == rhs.prefixLength) &&
                 ( address.EqualPrefix( rhs.address, prefixLength ) ) );
    }

    bool IPPrefix::operator < ( IPPrefix const & rhs ) const
    {
        if ( prefixLength != rhs.prefixLength ) 
        {
            return prefixLength < rhs.prefixLength;
        }

        return address.LessPrefix( rhs.address, prefixLength );
    }

    void IPPrefix::ToString( std::wstring& buffer ) const 
    {
        buffer.resize(INET6_ADDRSTRLEN + 5);
        address.ToString(buffer);
        buffer.append(L"/");
        buffer.append(std::to_wstring((int64)prefixLength ));
    }

    void IPPrefix::WriteTo(Common::TextWriter & w, FormatOptions const &) const 
    {
        std::wstring temp;
        ToString(temp);
        w.Write(temp);
    }

    void IPPrefix::GetV4MaskAsString( std::wstring& buffer ) const
    {
        debug_assert(this->address.AddressFamily == AddressFamily::InterNetwork); //IPv4
        int mask = CreateMask(this->prefixLength);
        Endpoint maskIp(mask);
        maskIp.ToString(buffer);
    }

#pragma prefast(push)
#pragma prefast(disable: 24002, "IPv4/v6 interop specific operation")
    void IPPrefix::AdjustFixedPrefixForTunnel(
        Common::Endpoint const & tunnelAddr, 
        Common::IPPrefix const & v4Prefix
        )
    {
        debug_assert(this->address.IsIPV6());
        debug_assert(v4Prefix.address.IsIPV4());
        debug_assert(tunnelAddr.IsISATAP() || tunnelAddr.Is6to4());
        debug_assert(DoesAddressMatchPrefix(tunnelAddr, *this));

        if (tunnelAddr.IsISATAP())
        {
            // Write the ISATAP identifier into the prefix, as we
            // find it in the tunnel address. This should be 
            // 0x0000 or 0x2000 and 0xfe5e.
            ((USHORT*)&this->address.as_sockaddr_in6()->sin6_addr)[4] = 
                ((USHORT*)&tunnelAddr.as_sockaddr_in6()->sin6_addr)[4];
            ((USHORT*)&this->address.as_sockaddr_in6()->sin6_addr)[5] = 
                ((USHORT*)&tunnelAddr.as_sockaddr_in6()->sin6_addr)[5];

            // Write the IPv4 prefix as the v4 address.
            KMemCpySafe(
                (UCHAR*)&this->address.as_sockaddr_in6()->sin6_addr + 12,
                sizeof(struct in_addr),
                (PVOID)&(v4Prefix.address.as_sockaddr_in()->sin_addr),
                sizeof(struct in_addr)
                );

            // Update the prefix length to include the ISATAP id and IPv4 prefix.
            this->prefixLength = 64 + 32 + v4Prefix.GetPrefixLength();

            ///TODO: remove
            //for serialization (temporary)
            address.ToString(temp_str_);
            temp_str_.append(L"/" + Int32_ToString(prefixLength));
        }
        else
        {
            // Write the IPv4 prefix as the v4 address.
            KMemCpySafe(
                (UCHAR*)&this->address.as_sockaddr_in6()->sin6_addr + 2,
                sizeof(struct in_addr),
                (PVOID)&(v4Prefix.address.as_sockaddr_in()->sin_addr),
                sizeof(struct in_addr)
                );

            ///TODO:remove
            //for serialization (temporary)
            address.ToString(temp_str_);
            temp_str_.append(L"/" + Int32_ToString(prefixLength));
        }
    }
#pragma prefast(pop)

    //
    // Free functions
    //
    bool AreAddressesOnTheSameSubnet(IPPrefix const & a, IPPrefix const & b)
    {
        if (a.GetPrefixLength() != b.GetPrefixLength())
            return false;

        return DoesAddressMatchPrefix( b.GetAddress(), a );
    }

    void PrefixIntersection ( IPPrefixSet const & prefixes1,
                              IPPrefixSet const & prefixes2,
                              IPPrefixSet       & intersection )
    {
        std::set_intersection( prefixes1.begin(), prefixes1.end(),
                               prefixes2.begin(), prefixes2.end(),
                               std::inserter( intersection, intersection.end() ) );
    }

    void PrefixUnion ( IPPrefixSet const & prefixes1,
                       IPPrefixSet const & prefixes2,
                       IPPrefixSet       & intersection )
    {
        std::set_union( prefixes1.begin(), prefixes1.end(),
                        prefixes2.begin(), prefixes2.end(),
                        std::inserter( intersection, intersection.end() ) );
    }

} // end namespace Common
