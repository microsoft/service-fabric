// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common {
    //
    // IPPrefixes are wierd.
    // They are not traditional prefixes - they are a combination
    // of regular IP addresses and prefix length
    // operator == and operator < are defined to do the correct thing
    // and compare only the prefix'd part of the address
    // However, GetAddress will return the entire IP address without
    // the lower bits being zero
    //
    struct IPPrefix  
    {
        IPPrefix () : prefixLength (0) {}

        explicit IPPrefix(
            Common::Endpoint const & address, 
            ULONG prefixLength
            ) :
            address( address ),
            prefixLength( prefixLength ),
            temp_str_(L"")
        {
            Sockets::Startup();
            this->address.GetIpString(temp_str_);
            temp_str_.append(L"/" + Int32_ToString(prefixLength));
        }

        bool operator == ( IPPrefix const & rhs ) const;

        bool IsLinkLocal() const
        {
            return address.IsLinkLocal();
        }

        bool IsSiteLocal () const
        {
            return address.IsSiteLocal();
        }

        bool IsV4SiteLocal() const
        {
            return IsIPV4() && IsSiteLocal();
        }

        bool IsV6SiteLocal() const
        {
            return IsIPV6() && IsSiteLocal();
        }

        bool IsIPV4() const
        { 
            return IsIPv4();
        }

        bool IsIPV6() const
        { 
            return IsIPv6();
        }

        //
        // The correct naming style. Deprecate the other
        //
        bool IsIPv4() const
        { 
            return address.IsIPV4();
        }

        bool IsIPv6() const
        { 
            return address.IsIPV6();
        }

        ULONG GetPrefixLength() const
        {
            return prefixLength;
        }

        bool operator < (IPPrefix const & rhs) const;


        // NOT TESTED
        //static void GetPrefixDifferenceSet(std::set<IPPrefix> const &set1,
        //    std::set<IPPrefix> const & set2,
        //    std::set<IPPrefix> & outputSet)
        //{
        //    std::insert_iterator<std::set<IPPrefix> > p(outputSet, outputSet.begin());
        //    std::set_difference( 
        //        set1.begin(), set1.end(),
        //        set2.begin(), set2.end(),
        //        p, less_IPPrefix() );
        //}

        void ToString( std::wstring& buffer ) const ;

        void WriteTo(TextWriter & w, FormatOptions const &) const;


//         Endpoint const & get_Address() const
//         {
//             return this->address;
//         }

        Endpoint const & GetAddress() const
        {
            return this->address;
        }

        void GetV4MaskAsString( std::wstring& buffer ) const;

        void AdjustFixedPrefixForTunnel(Common::Endpoint const & tunnelAddr, Common::IPPrefix const & v4Prefix);

        //
        // Zero out all the bits that are not part of the prefix
        // So for example:
        // IPPrefix( Endpoint( L"fe80::1:2:3:4" ), 112 );
        // would have the address component equal
        // fe80::1:2:3:4.
        // However, after ZeroNonPrefixBits(), the address component
        // should be
        // fe80::1:2:3:0
        //
        void ZeroNonPrefixBits ();

        // friend bool DoesAddressMatchPrefix( Endpoint const &, IPPrefix const & );
        static std::wstring GetStringNotation(IPPrefix const & prefix){return prefix.temp_str_;}
        static IPPrefix GetIPPrefix(std::wstring const & prefix)
        {
            std::wstring ipAddress, prefixLength;
            auto index = prefix.find(L"/");
            ipAddress = prefix.substr(0, index);
            prefixLength = prefix.substr(index + 1);
            return IPPrefix(Endpoint(ipAddress), Common::Int32_Parse(prefixLength));
        }
        
    private:

        
        ///TODO: temporary code.remove
        inline std::wstring Int32_ToString(int value)
        {
            std::wstring result;
            StringWriter(result).Write(value);
            return result;
        }
        
        Endpoint address;
        ULONG prefixLength;
        std::wstring temp_str_;

    }; // end class IPPrefix

    //
    // Defined in IPAddressPrefix.cpp
    //
    namespace prefix {
        extern const IPPrefix IPv4LinkLocal;
        extern const IPPrefix IPv4SiteLocal1;
        extern const IPPrefix IPv4SiteLocal2;
        extern const IPPrefix IPv4SiteLocal3;

        extern const IPPrefix IPv6LinkLocal;
        extern const IPPrefix IPv6SiteLocal;        
    };

    typedef std::set<IPPrefix> IPPrefixSet;

    //////////////////////////////////////////////////////////////////////
    //
    // Utility functions
    //
    //////////////////////////////////////////////////////////////////////
    void PrefixIntersection ( IPPrefixSet const & prefixes1,
                              IPPrefixSet const & prefixes2,
                              IPPrefixSet       & intersection );

    void PrefixUnion ( IPPrefixSet const & prefixes1,
                       IPPrefixSet const & prefixes2,
                       IPPrefixSet       & intersection );

    bool DoesAddressMatchPrefix(Endpoint const & address, IPPrefix const & prefix);
    bool AreAddressesOnTheSameSubnet(IPPrefix const & a, IPPrefix const & b);

    struct DoesAddressMatchPrefixWrapper
    {
        explicit DoesAddressMatchPrefixWrapper ( Endpoint const & endpoint )
            : lhs( endpoint )
        {
        }

        bool operator () ( IPPrefix const & rhs )
        {
            return DoesAddressMatchPrefix( lhs, rhs );
        }
    
        Endpoint lhs;
    };

    //////////////////////////////////////////////////////////////////////
    //
    // Find Matching Prefix
    //
    // Templatized way to iterate over a list or vector of IPPrefiPtrs and
    // find the one that matches. Returns a nullptr if there is no match
    //
    //////////////////////////////////////////////////////////////////////
    //template < typename Container >
    //IPPrefix FindMatchingPrefix ( ConstIterator begin, ConstIterator end,
    //                                 Endpoint const & address ) 
    //{
    //    for ( ConstIterator pos = begin; pos != end; ++pos ) {
    //        if ( DoesAddressMatchPrefix( address, **pos ) ) {
    //            return pos;
    //        }
    //    }

    //    return nullptr;
    //}

    typedef std::set<Common::IPPrefix> PrefixSet;
    
} // end namespace Common

