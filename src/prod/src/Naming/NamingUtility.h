// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingUtility
    {
        DENY_COPY(NamingUtility);

    public:
        static bool ExtractMemberServiceLocation(
            ResolvedServicePartitionSPtr const & serviceGroupLocation,
            std::wstring const & serviceName,
            __out ResolvedServicePartitionSPtr & serviceMemberLocation);

        static void SetCommonTransportProperties(Transport::IDatagramTransport & transport);

        static size_t GetMessageContentThreshold();

        static Common::ErrorCode CheckEstimatedSize(size_t estimatedSize);
        static Common::ErrorCode CheckEstimatedSize(size_t estimatedSize, size_t maxAllowedSize);

        static size_t GetHash(std::wstring const &);

        static bool ValidateClientMessage(Transport::MessageUPtr const &, Common::StringLiteral const &traceComponent, std::wstring const &instanceString);
        static Common::ErrorCode CheckClientTimeout(Common::TimeSpan const &clientTimeout, Common::StringLiteral const &traceComponent, std::wstring const &instanceString);

        static std::wstring GetFabricGatewayServiceName(std::wstring const &nodeName);
        static std::wstring GetFabricApplicationGatewayServiceName(std::wstring const &nodeName);

    private:
        static Common::Global<std::locale> USLocale;
        
        static bool ParseServiceGroupAddress(
            std::wstring const & serviceGroupAddress,
            std::wstring const & pattern,
            __out std::wstring & memberServiceAddress);

        static bool ExtractServiceRoutingAgentHeader(Transport::MessageUPtr const & receivedMessage, Common::StringLiteral const &traceComponent, __out SystemServices::ServiceRoutingAgentHeader & routingAgentHeader);
    };
}
