// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessServiceExistsRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessServiceExistsRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( GetServiceDescription )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr && request);
        void PerformRequest(Common::AsyncOperationSPtr const &);  

    private:
        void HandleRequestFromCache(Common::AsyncOperationSPtr const &);  
        void HandleRequestFromDisk(Common::AsyncOperationSPtr const &);  

        Common::ErrorCode GetUserServiceMetadata(
            __out UserServiceState::Enum &, 
            __out PartitionedServiceDescriptor & metadata);

        Common::ErrorCode InternalGetUserServiceStateAtNameOwner(
            TransactionSPtr const &, 
            __out UserServiceState::Enum & serviceState); 

        void SetServiceDoesNotExistReply();
    };
}
