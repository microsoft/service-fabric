// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessInnerUpdateServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessInnerUpdateServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            StoreServiceProperties &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_UpdateDescription)) ServiceUpdateDescription const & UpdateDescription;
        ServiceUpdateDescription const & get_UpdateDescription() const { return updateDescription_; }

        void OnCompleted();

        DEFINE_PERF_COUNTERS ( NOUpdateService )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:
        void StartUpdateService(Common::AsyncOperationSPtr const &);

        void StartRequestToFM(Common::AsyncOperationSPtr const &);

        void OnRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishUpdateService(Common::AsyncOperationSPtr const &);

        void OnWriteServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Common::ErrorCode GetUserServiceMetadata(__out PartitionedServiceDescriptor &);

        Common::ErrorCode TryUpdateServiceDescription(__out bool & updated);

        ServiceUpdateDescription updateDescription_;
        PartitionedServiceDescriptor psd_;
        std::vector<Reliability::ConsistencyUnitDescription> addedCuids_;
        std::vector<Reliability::ConsistencyUnitDescription> removedCuids_;
    };
}

