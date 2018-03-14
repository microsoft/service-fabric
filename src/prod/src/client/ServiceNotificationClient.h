// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ServiceNotificationClient 
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
    {
    private:
        static const ClientEventSource Trace;

        typedef std::function<Common::ErrorCode(std::vector<ServiceNotificationResultSPtr> const &)> NotificationHandler;
        typedef std::pair<uint64, Reliability::ServiceNotificationFilterSPtr> RegisteredFilterPair;
        typedef std::map<uint64, Reliability::ServiceNotificationFilterSPtr> RegisteredFilterMap;
        
    public:
        ServiceNotificationClient(
            std::wstring const & clientId, 
            INotificationClientSettingsUPtr &&,
            __in ClientConnectionManager &,
            __in INotificationClientCache &,
            NotificationHandler const &,
            Common::ComponentRoot const &);

        ~ServiceNotificationClient();

        __declspec (property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return clientId_; }

        __declspec (property(get=get_ClientId)) std::wstring const & ClientId;
        std::wstring const & get_ClientId() const { return clientId_; }

        __declspec (property(get=get_CurrentGateway)) Naming::GatewayDescription const & CurrentGateway;
        Naming::GatewayDescription const & get_CurrentGateway() const 
        { 
            Common::AcquireReadLock lock(lifecycleLock_); 
            return currentGateway_; 
        }

        __declspec (property(get=get_RegisteredFilters))  RegisteredFilterMap const & RegisteredFilters;
        RegisteredFilterMap const & get_RegisteredFilters() const { return registeredFilters_; }

        __declspec (property(get=get_ClientSettings)) INotificationClientSettings const & ClientSettings;
        INotificationClientSettings const & get_ClientSettings() const { return *settings_; }

        Common::AsyncOperationSPtr BeginRegisterFilter(
            Common::ActivityId const &,
            Reliability::ServiceNotificationFilterSPtr const & filter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterFilter(
            Common::AsyncOperationSPtr const & operation,
            __out uint64 & filterId);

        Common::AsyncOperationSPtr BeginUnregisterFilter(
            Common::ActivityId const &,
            uint64 filterId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUnregisterFilter(
            Common::AsyncOperationSPtr const & operation);

    protected:

        //
        // FabricComponent
        //
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class RequestAsyncOperationBase;
        class GatewaySynchronizationContext;
        class SynchronizeSessionAsyncOperation;
        class FilterAsyncOperationBase;
        class RegisterFilterAsyncOperation;
        class UnregisterFilterAsyncOperation;
        class ProcessNotificationAsyncOperation;

        uint64 GetNextFilterId();

        void AddFilter(
            Reliability::ServiceNotificationFilterSPtr const & filter);

        void RemoveFilter(uint64 filterId);

        void UpdateVersionsCallerHoldsLock(
            Reliability::GenerationNumber const &,
            Common::VersionRangeCollectionSPtr &&);

        bool TryDrainPendingNotificationsOnSynchronized(
            Common::ActivityId const &,
            Naming::GatewayDescription const & targetConnectedGateway);

        void AcceptAndPostNotificationsIfSynchronized(
            Naming::ServiceNotificationSPtr &&,
            std::vector<ServiceNotificationResultSPtr> &&);

        std::vector<ServiceNotificationResultSPtr> ComputeAcceptedNotificationsCallerHoldsLock(
            PendingServiceNotificationResultsItemSPtr &&);

        void TryAcceptNotification(
            __in std::vector<ServiceNotificationResultSPtr> & updates,
            Naming::ServiceNotificationPageId const &,
            ServiceNotificationResultSPtr const &);

        void PostNotifications(
            std::vector<ServiceNotificationResultSPtr> &&);

        void OnNotificationReceived(
            __in Transport::Message & request,
            __in Transport::ReceiverContextUPtr & receiverContext);

        void OnProcessNotificationComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void OnGatewayConnected(
            ClientConnectionManager::HandlerId,
            Transport::ISendTarget::SPtr const &,
            Naming::GatewayDescription const &);

        void StartSynchronizingSession(
            Naming::GatewayDescription const & targetGateway,
            Common::ActivityId const &);

        void OnSynchronizingSessionComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        std::wstring clientId_;
        INotificationClientSettingsUPtr settings_;
        ClientConnectionManager & clientConnection_;
        INotificationClientCache & clientCache_;

        NotificationHandler notificationHandler_;
        Common::RwLock handlerLock_;

        // lifecycleLock_
        //
        Naming::GatewayDescription targetGateway_;
        Naming::GatewayDescription currentGateway_;
        uint64 nextFilterId_;
        std::map<uint64, Reliability::ServiceNotificationFilterSPtr> registeredFilters_;
        Reliability::GenerationNumber generation_;
        Common::VersionRangeCollectionSPtr versions_;
        std::unique_ptr<GatewaySynchronizationContext> gatewaySynchronizationContext_;
        std::vector<PendingServiceNotificationResultsItemSPtr> pendingNotifications_; bool isSynchronized_;
        mutable Common::RwLock lifecycleLock_;

        ClientConnectionManager::HandlerId connectionHandlersId_;
    };

    typedef std::unique_ptr<ServiceNotificationClient> ServiceNotificationClientUPtr;
}
