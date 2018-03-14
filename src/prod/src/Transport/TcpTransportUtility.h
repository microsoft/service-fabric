// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    // todo, leikong, remove ResolveOptions as it is a duplicate of Common::AddressFamily, need to move TryParse and WriteTo over
    namespace ResolveOptions
    {
        enum Enum
        {
            Invalid = -1,
            Unspecified = AF_UNSPEC,
            IPv4 = AF_INET,
            IPv6 = AF_INET6
        };

        _Success_(return)
        bool TryParse(std::wstring const & input, _Out_ Enum & output);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }

    class TcpTransportUtility : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
    public:
        static Common::ErrorCode GetLocalFqdn(std::wstring & hostname);

        static Common::ErrorCode ResolveToAddresses(
            std::wstring const & hostName,
            ResolveOptions::Enum options,
            _Out_ std::vector<Common::Endpoint> & resolved);

        static Common::ErrorCode GetLocalAddresses(
            _Out_ std::vector<Common::Endpoint> & localAddresses,
            ResolveOptions::Enum options = ResolveOptions::Unspecified);

        static Common::ErrorCode GetFirstLocalAddress(
            _Out_ Common::Endpoint & localAddress,
            ResolveOptions::Enum options = ResolveOptions::Unspecified);

        static bool IsValidEndpointString(std::wstring const & address);

        static Common::Endpoint ParseEndpointString(std::wstring const & address);

        static std::wstring ParseHostString(std::wstring const & address);

        static USHORT ParsePortString(std::wstring const & address);

        static Common::ErrorCode TryParseEndpointString(std::wstring const & address, Common::Endpoint & endpoint);

        static Common::ErrorCode TryParseHostPortString(std::wstring const & address, std::wstring & host, std::wstring & port);

        _Success_(return)
        static bool TryParsePortString(std::wstring const & portString, _Out_ USHORT & port);

        static bool IsLoopbackAddress(std::wstring const & address);

        // Note: local != loopback
        static bool IsLocalEndpoint(Common::Endpoint const & endpoint);

        _Success_(return)
        static bool TryParseHostNameAddress(
            std::wstring const & hostnameAddress,
            _Out_ std::wstring & hostnameResult,
            _Out_ USHORT & portResult);

        static Common::ErrorCode TryResolveHostNameAddress(
            std::wstring const & hostnameAddress,
            ResolveOptions::Enum addressType,
            _Out_ std::vector<Common::Endpoint> & endpoints);

        static void EnableTcpFastLoopbackIfNeeded(
            Common::Socket & socket,
            std::wstring const & traceId);

        static std::wstring TcpTransportUtility::ConstructAddressString(
            std::wstring const & ipAddressOrFQDN,
            uint port);
   };
}
