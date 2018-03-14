// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ServiceLocationNotificationAsyncOperation : 
        public EntreeService::RequestAsyncOperationBase
    {
    public:
        ServiceLocationNotificationAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        ~ServiceLocationNotificationAsyncOperation();
        
    protected:
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;
       
        virtual void OnCompleted();

        virtual void OnTimeout(Common::AsyncOperationSPtr const & thisSPtr);

        virtual bool CanConsiderSuccess();
               
    private:

        static Common::TimeSpan TimeoutWithoutSendBuffer(Common::TimeSpan const timeout);
        
        void ProcessNextRequestData(Common::AsyncOperationSPtr const &);

        void OnGetServiceDescriptionComplete(
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        void ProcessPsdForCurrentRequestData(
            Common::AsyncOperationSPtr const &,
            PartitionedServiceDescriptorSPtr const &,
            __int64 storeVersion,
            Common::ErrorCode const &);

        void WaitForNextBroadcast(Common::AsyncOperationSPtr const &);

        void OnBroadcastNotification(
            UserServiceCuidMap const &,
            Common::AsyncOperationSPtr const &);

        void AddBroadcastRequest(
            Common::NamingUri const &,
            Reliability::ConsistencyUnitId const &);

        bool TryUnregisterBroadcastRequests(
            Common::AsyncOperationSPtr const &);

        bool TryAddCachedServicePartitionToReply(
            ServiceLocationNotificationRequestData const &,
            PartitionedServiceDescriptorSPtr const &,
            Reliability::ConsistencyUnitId const &);

        bool TryAddFailureToReply(
            ServiceLocationNotificationRequestData const &,
            Common::ErrorCodeValue::Enum const,
            __int64 storeVersion);

        void CompleteWithCurrentResultsOrError(
            Common::AsyncOperationSPtr const &,
            Common::ErrorCode const &);

        void CompleteWithCurrentResults(
            Common::AsyncOperationSPtr const &);

        // The requests received from the client.
        // This are expected to be sorted by Common::NamingUri.
        // Multiple entries can exist for the same name, for different partition keys
        std::vector<ServiceLocationNotificationRequestData> requestData_;

        // Tracks the index of the current ServiceLocationNotificationRequestData being processed.
        size_t requestDataIndex_;
        std::unique_ptr<GatewayNotificationReplyBuilder> replyBuilder_;
        
        // Tracks all relevant CUIDs for broadcast notifications if no
        // updates were picked up during processing.
        BroadcastRequestVector broadcastRequests_;

        // Names registered for broadcast updates.
        // They will be unregistered when the async operation completes,
        // either because a broadcast brings newer information for the client
        // or an error occurred (eg timeout)
        std::set<Common::NamingUri> broadcastRegisteredNames_;

        // Set after the first broadcast has been received
        bool firstBroadcastReceived_;

        // Synchronize registering/unregistering for broadcasts
        Common::ExclusiveLock broadcastLock_;

        // Used to ensure that only a single broadcast is processed
        // at a time (only one processing workflow can be active at a time).
        bool waitingForBroadcast_;

        // On timeout, use the field to determine whether there was an error
        // or there were simply no updates to report.
        bool noNewerData_;
    };
}
