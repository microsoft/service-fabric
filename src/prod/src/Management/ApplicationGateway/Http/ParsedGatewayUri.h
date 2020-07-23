// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class ParsedGatewayUri : public Common::AllocableObject
    {
        DENY_COPY(ParsedGatewayUri)
    public:

        static bool TryParse(HttpServer::IRequestMessageContextUPtr &messageContext, __out ParsedGatewayUriSPtr &parsedGatewayUriSPtr, std::wstring serviceName);

        Common::ErrorCode TryGetServiceName(__out Common::NamingUri &serviceName);

        bool GetUriForForwarding(
            std::wstring const &serviceName, 
            std::wstring const &serviceListenAddress, 
            __out std::wstring &forwardingUri);

        __declspec(property(get = get_UrlSuffix)) std::wstring const &RequestUrlSuffix;
        std::wstring const& get_UrlSuffix() const{ return urlSuffix_; }

        __declspec(property(get = get_PartitionKey)) Naming::PartitionKey const &PartitionKey;
        Naming::PartitionKey const& get_PartitionKey() const{ return partitionKey_; }

        __declspec(property(get = get_RequestTimeout)) Common::TimeSpan const &RequestTimeout;
        Common::TimeSpan const& get_RequestTimeout() const{ return requestTimeout_; }

        __declspec(property(get = get_TargetReplicaSelector)) TargetReplicaSelector::Enum const &TargetReplicaSelector;
        TargetReplicaSelector::Enum const& get_TargetReplicaSelector() const{ return targetReplicaSelector_; }

        __declspec(property(get = get_ListenerName)) std::wstring const &ListenerName;
        std::wstring const& get_ListenerName() const{ return listenerName_; }

        __declspec(property(get = get_ServiceName)) std::wstring const &ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

    private:
        ParsedGatewayUri(Common::Uri uri)
            : uri_(uri)
            , requestTimeout_(Common::TimeSpan::Zero)
            , partitionKey_()
            , serviceNameQueryParam_()
            , targetReplicaSelector_(TargetReplicaSelector::Default)
            , listenerName_()
            , serviceName_()
        {}

        bool TryParseQueryParams(std::wstring const &queryString);
        bool TryParsePartitionKindQueryParam(std::wstring const &partitionKindQueryParam, std::wstring const &partitionKeyQueryParam);

        void ParseSuffix(std::wstring const &suffixElement);
        void ParseHttpGatewayUriSuffix(std::wstring const &suffixElement);

        Common::Uri uri_;
        Common::TimeSpan requestTimeout_;
        std::wstring urlSuffix_;
        std::wstring parsedQueryString_;
        Naming::PartitionKey partitionKey_;
        std::wstring serviceNameQueryParam_;
        TargetReplicaSelector::Enum targetReplicaSelector_;
        std::wstring listenerName_;
        std::wstring serviceName_;
    };
}
