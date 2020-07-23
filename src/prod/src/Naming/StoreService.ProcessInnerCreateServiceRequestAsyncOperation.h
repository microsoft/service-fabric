// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessInnerCreateServiceRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessInnerCreateServiceRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_ServiceDescriptor)) PartitionedServiceDescriptor const & ServiceDescriptor;
        PartitionedServiceDescriptor const & get_ServiceDescriptor() const { return psd_; }

        __declspec(property(get=get_MutableServiceDescriptor, put=set_MutableServiceDescriptor)) PartitionedServiceDescriptor & MutableServiceDescriptor;
        PartitionedServiceDescriptor & get_MutableServiceDescriptor() { return psd_; }
        void set_MutableServiceDescriptor(PartitionedServiceDescriptor && value) { psd_ = std::move(value); }

        void OnCompleted();

        DEFINE_PERF_COUNTERS ( NOCreateService )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:
        void StartCreateService(Common::AsyncOperationSPtr const &);

        void OnTentativeWriteComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void StartRequestToFM(Common::AsyncOperationSPtr const &);

        void OnRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void GetServiceDescriptionFromFM(Common::AsyncOperationSPtr const &);

        void OnGetServiceDescriptionComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        
        void FinishCreateService(Common::AsyncOperationSPtr const &);

        void RevertTentativeCreate(Common::AsyncOperationSPtr const &);

        void OnRevertTentativeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void OnWriteServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        mutable PartitionedServiceDescriptor psd_;

        bool isRebuildFromFM_;

        Common::ErrorCode revertError_;
    };
}
