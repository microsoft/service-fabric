// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    //
    // The purpose of the class is to track the notifications and raise callbacks appropriately.
    // The notification requests are put in a map based on the (servicename, partition key) pair.
    // For one of this pair, multiple callbacks may be registered. 
    // Errors currently supported for address detection: UserServiceNotFound, InvalidServicePartition and
    // AddressDenied (for service groups).
    // To get the updates, each tracker builds a request using the latest data it has.
    // If the fabric client has something newer in its cache, the requests will be served using the cached entries.
    // Otherwise, the requests go to the gateway.
    // When an update comes, the manager finds the trackers for which the update is relevant and then pass it to them.
    // The trackers determine whether the update is newer than what they already have, 
    // and they raise the callbacks with this update as appropriate.
    //
    class ServiceAddressTrackerManager : public Common::RootedObject
    {
        DENY_COPY(ServiceAddressTrackerManager);

    public:
        explicit ServiceAddressTrackerManager(__in FabricClientImpl & client);

        ~ServiceAddressTrackerManager();

        void CacheUpdatedCallback(
            Naming::ResolvedServicePartitionSPtr const & update, 
            Naming::AddressDetectionFailureSPtr const & failure,
            Transport::FabricActivityHeader const & activityHeader);

        Common::ErrorCode AddTracker(
            Api::ServicePartitionResolutionChangeCallback const &handler,
            Common::NamingUri const& name,
            Naming::PartitionKey const& partitionKey,
            __out LONGLONG & handlerId);

        bool RemoveTracker(LONGLONG handlerId);

        void CancelPendingRequests();

    private:
        void PollServiceLocations(
            Transport::FabricActivityHeader && activityHeader);

        void FinishPoll(
            Common::AsyncOperationSPtr const & pollOperation,
            bool expectedCompletedSynchronously);

        void CheckIfAllPendingPollRequestsDone(uint64 pendingOperationCount);

        void PostPartitionUpdateCallerHoldsLock(
            Naming::ResolvedServicePartitionSPtr const & partition,
            Transport::FabricActivityHeader const & activityHeader);

        void PostFailureUpdateCallerHoldsLock(
            Naming::AddressDetectionFailureSPtr const & failure,
            Transport::FabricActivityHeader const & activityHeader);
               
        static bool IsRetryableError(Common::ErrorCode const error);

        static bool ShouldRaiseError(Common::ErrorCode const error);

        void AddRequestDataCallerHoldsLock(
            ServiceAddressTrackerSPtr const & tracker,
            Transport::FabricActivityHeader const & activityHeader,
            __in std::vector<ServiceAddressTrackerPollRequests> & batchedPollRequests);

        // Shows whether a poll operation is in progress
        bool hasPendingRequest_;
        Common::RwLock lock_;

        // Map with trackers registered for a <service name, partition key> pair
        // Each tracker can contain multiple callbacks registered for that tuple.
        // For service groups, we can have multiple entries for the same base name,
        // and each tracker maps to the full service group name.
        typedef std::pair<Naming::NameRangeTuple, ServiceAddressTrackerSPtr> NameRangeTupleTrackerPair;
        typedef std::multimap<Naming::NameRangeTuple, ServiceAddressTrackerSPtr, Naming::NameRangeTuple::LessThanComparitor> NameRangeTupleTrackerMap;
        NameRangeTupleTrackerMap trackers_;

        Common::TimerSPtr timer_;
        bool cancelled_;

        FabricClientImpl & client_;
        FabricClientInternalSettingsHolder settingsHolder_;

        // If many requests are registered, they may not fit in one message to be sent to the gateway.
        // Break the requests in multiple messages and start a poll for each of them, 
        // then wait for all the polls to be completed before starting the next ones.
        std::vector<ServiceAddressTrackerPollRequests> batchedPollRequests_;
        Common::atomic_uint64 pendingBatchedPollCount_;
        bool startPollImmediately_;

        // If many requests are registered, it's possible that not all updates fit in one message
        // Mark what was the last non-processed request, so future requests will ask for the remaining changes first.
        std::unique_ptr<Naming::NameRangeTuple> firstNonProcessedRequest_;
    };
}
