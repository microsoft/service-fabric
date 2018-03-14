// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::LocationChangeNotificationAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
        DENY_COPY(LocationChangeNotificationAsyncOperation)

    public:
        LocationChangeNotificationAsyncOperation(
            __in FabricClientImpl & client,
            std::vector<Naming::ServiceLocationNotificationRequestData> && requests,
            Transport::FabricActivityHeader && activityHeader,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            __in_opt Common::ErrorCode && passThroughError = Common::ErrorCode::Success());

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<Naming::ResolvedServicePartitionSPtr> & partitions, 
            __out std::vector<Naming::AddressDetectionFailureSPtr> & failures,
            __out bool & updateNonProcessedRequest,
            __out std::unique_ptr<Naming::NameRangeTuple> & firstNonProcessedRequest,
            __out Transport::FabricActivityHeader & activityHeader);

    protected:
        void OnStartOperation(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void OnRequestComplete(
            Common::AsyncOperationSPtr const & requestOp,
            bool expectedCompletedSynchronously);

        void OnUpdateCacheEntriesComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        bool ShouldAddEntry(
            Naming::ResolvedServicePartitionSPtr const & entry,
            Naming::ServiceLocationNotificationRequestData const & request);

        bool TryAddToReply(
            Naming::ResolvedServicePartitionSPtr && entry,
            Naming::ServiceLocationNotificationRequestData const & request,
            __in std::vector<Naming::ResolvedServicePartitionSPtr> & partitions);
    
        FabricClientInternalSettingsHolder settingsHolder_;
        std::vector<Naming::ServiceLocationNotificationRequestData> requests_;
        std::vector<Naming::ResolvedServicePartitionSPtr> partitions_;
        std::vector<Naming::AddressDetectionFailureSPtr> failures_;
        bool updateNonProcessedRequest_;
        std::unique_ptr<Naming::NameRangeTuple> firstNonProcessedRequest_;
        std::set<Reliability::ConsistencyUnitId> currentCuids_;
    };
}
