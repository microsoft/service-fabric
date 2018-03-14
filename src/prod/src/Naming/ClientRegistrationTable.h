// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ClientRegistrationId
    {
    public:
        ClientRegistrationId() : id_() { }
        ClientRegistrationId(ClientIdentityHeader const & clientIdentity) : id_(clientIdentity.TargetId) { }

        bool operator < (ClientRegistrationId const & other) const { return id_ < other.id_; }
        bool operator <= (ClientRegistrationId const & other) const { return id_ <= other.id_; }
        bool operator > (ClientRegistrationId const & other) const { return id_ > other.id_; }
        bool operator >= (ClientRegistrationId const & other) const { return id_ >= other.id_; }
        bool operator == (ClientRegistrationId const & other) const { return id_ == other.id_; }
        bool operator != (ClientRegistrationId const & other) const { return id_ != other.id_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const { w << id_; }

    private:
        std::wstring id_;
    };

    class ClientRegistration 
        : public Reliability::IClientRegistration
        , public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(ClientRegistration)

    private:

        typedef std::pair<uint64, Reliability::ServiceNotificationFilterSPtr> FilterPair;
        typedef std::map<uint64, Reliability::ServiceNotificationFilterSPtr> FiltersMap;

        typedef std::function<void(ClientRegistrationId const &)> RegistrationFaultHandler;

    public:

        ClientRegistration();

        ClientRegistration(
            std::wstring const & traceId,
            EntreeServiceTransportSPtr const &,
            ClientIdentityHeader const &,
            std::wstring && clientId,
            Common::VersionRangeCollectionSPtr const & initialClientVersions,
            ServiceNotificationSender::TargetFaultHandler const &);

        __declspec (property(get=get_Id)) ClientRegistrationId const & Id;
        ClientRegistrationId const & get_Id() const { return registrationId_; }

        __declspec (property(get=get_ClientId)) std::wstring const & ClientId;
        std::wstring const & get_ClientId() const { return clientId_; }

        __declspec (property(get=get_Filters)) FiltersMap const & Filters;
        FiltersMap const & get_Filters() const { return filtersById_; }

        Common::ErrorCode TryAddFilter(Reliability::ServiceNotificationFilterSPtr const & filter);

        Reliability::ServiceNotificationFilterSPtr TryRemoveFilter(uint64 filterId);

        void StartSender();

        void StopSender();

        void CreateAndEnqueueNotification(
            Common::ActivityId const &,
            Reliability::GenerationNumber const & cacheGeneration,
            Common::VersionRangeCollectionSPtr && cacheVersions,
            Reliability::MatchedServiceTableEntryMap && matchedPartitions,
            Common::VersionRangeCollectionSPtr && updateVersions);

        void CreateAndEnqueueNotificationForClientSynchronization(
            Common::ActivityId const &,
            Reliability::GenerationNumber const & cacheGeneration,
            Common::VersionRangeCollectionSPtr && cacheVersions,
            Reliability::MatchedServiceTableEntryMap && matchedPartitions,
            Common::VersionRangeCollectionSPtr && updateVersions);

        bool IsConnectionProcessingComplete();
        void SetConnectionProcessingComplete();

    public:

        //
        // IClientRegistration
        //

        virtual bool IsSenderStopped() const;

    private:
        ServiceNotificationSenderSPtr notificationSender_;
        std::wstring clientId_;

        ClientRegistrationId registrationId_;
        FiltersMap filtersById_;

        bool isConnectionProcessingComplete_;
    };

    typedef std::shared_ptr<ClientRegistration> ClientRegistrationSPtr;

    class ClientRegistrationTable
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(ClientRegistrationTable)

    public:

        typedef std::pair<ClientRegistrationId, ClientRegistrationSPtr> RegistrationPair;
        typedef std::map<ClientRegistrationId, ClientRegistrationSPtr> RegistrationsMap;

    public:

        ClientRegistrationTable();

        void Clear();

        bool TryAddOrGetExistingRegistration(__inout ClientRegistrationSPtr & registration);

        ClientRegistrationSPtr TryGetRegistration(
            ClientIdentityHeader const & clientIdentity, 
            std::wstring const & clientId);

        ClientRegistrationSPtr TryGetRegistration(
            ClientRegistrationId const & registrationId, 
            std::wstring const & clientId);
      
        ClientRegistrationSPtr TryRemoveRegistration(
            ClientIdentityHeader const & clientIdentity, 
            std::wstring const & clientId);

        ClientRegistrationSPtr TryRemoveRegistration(
            ClientRegistrationId const & registrationId, 
            std::wstring const & clientId);

    private:

        RegistrationsMap registrations_;
    };
}
