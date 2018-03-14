// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotificationManager 
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(ServiceNotificationManager)

    public:

        typedef std::pair<ClientRegistrationId, Reliability::MatchedServiceTableEntrySPtr> MatchResult;
        typedef std::vector<MatchResult> MatchResultList;

    public:
        ServiceNotificationManager(
            Federation::NodeInstance const &,
            std::wstring const & nodeName,
            Reliability::IServiceTableEntryCache &, 
            GatewayEventSource const &,
            Common::ComponentRoot const &);

	~ServiceNotificationManager();

        __declspec (property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec (property(get=get_Transport)) EntreeServiceTransportSPtr const & Transport;
        EntreeServiceTransportSPtr const & get_Transport() const { return transport_; }

        Common::ErrorCode Open(EntreeServiceTransportSPtr const &);

        Common::ErrorCode ProcessRequest(
            Transport::MessageUPtr const & request,
            __out Transport::MessageUPtr & reply);

        GatewayDescription CreateGatewayDescription() const;

    protected:

        //
        // FabricComponent
        //
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:

        class NameFilterIndex;

        Common::ErrorCode RegisterFilters(
            Common::ActivityId const &,
            ClientRegistrationSPtr const &,
            std::vector<Reliability::ServiceNotificationFilterSPtr> const &);
        
        Common::ErrorCode RegisterFilter(
            Common::ActivityId const &,
            ClientIdentityHeader const &,
            std::wstring && clientId,
            Reliability::ServiceNotificationFilterSPtr const &);

        Common::ErrorCode RegisterFilterCallerHoldsLock(
            ClientRegistrationSPtr const &,
            Reliability::ServiceNotificationFilterSPtr const &);

        bool UnregisterFilter(
            Common::ActivityId const &,
            ClientIdentityHeader const &,
            std::wstring const & clientId,
            uint64);

        Common::ErrorCode ConnectClient(
            Common::ActivityId const &,
            ClientIdentityHeader const &,
            std::wstring && clientId,
            Reliability::GenerationNumber const & clientGeneration,
            Common::VersionRangeCollectionSPtr && clientVersions,
            std::vector<Reliability::ServiceNotificationFilterSPtr> &&,
            __out Reliability::GenerationNumber & cacheGeneration,
            __out int64 & lastDeletedEmptyPartitionVersion);

        Common::ErrorCode SynchronizeClient(
            Common::ActivityId const &,
            ClientIdentityHeader const &,
            std::wstring && clientId,
            Reliability::GenerationNumber const & clientGeneration,
            std::vector<Reliability::VersionedConsistencyUnitId> && partitionsToCheck,
            __out Reliability::GenerationNumber & cacheGeneration,
            __out std::vector<int64> & deletedVersions);

        void RemoveClientRegistrations();
        void RemoveClientRegistration(ClientIdentityHeader const &);
        void RemoveClientRegistrationCallerHoldsLock(ClientIdentityHeader const &);

        void OnConnectionFault(Common::ErrorCode const &);

        void ProcessServiceTableEntries(
            Common::ActivityId const &,
            Reliability::GenerationNumber const &, 
            Common::VersionRangeCollection const & cacheVersions, 
            std::vector<Reliability::CachedServiceTableEntrySPtr> const &,
            Common::VersionRangeCollection const & updateVersions);

        bool TryRegisterNewClient(
            ClientIdentityHeader const &clientRegn,
            std::wstring && clientId,
            __out ClientRegistrationSPtr & registration);

        void ProcessServiceNotificationFilters(
            Common::ActivityId const &,
            ClientRegistrationSPtr const &,
            Reliability::GenerationNumber const & clientGeneration,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            std::vector<Reliability::ServiceNotificationFilterSPtr> const & filters,
            __out Reliability::GenerationNumber & cacheGeneration,
            __out int64 & lastDeletedEmptyPartitionVersion);

        MatchResultList GetExactMatches(
            Common::NamingUri const & uri,
            Reliability::CachedServiceTableEntrySPtr const &);

        MatchResultList GetPrefixMatches(
            Common::NamingUri const & uri,
            Reliability::CachedServiceTableEntrySPtr const &);

        std::unique_ptr<NameFilterIndex> const & GetNameFilterIndex(Reliability::ServiceNotificationFilter const &) const;

        Federation::NodeInstance nodeInstance_;
        std::wstring nodeName_;
        std::wstring traceId_;

        Reliability::IServiceTableEntryCache & serviceTableEntryCache_;
        GatewayEventSource const & trace_;
        EntreeServiceTransportSPtr transport_;

        std::unique_ptr<ClientRegistrationTable> clientTable_;
        std::unique_ptr<NameFilterIndex> exactFilterIndex_;
        std::unique_ptr<NameFilterIndex> prefixFilterIndex_;
        Common::RwLock registrationsLock_;
    };

    typedef std::unique_ptr<ServiceNotificationManager> ServiceNotificationManagerUPtr;
}

