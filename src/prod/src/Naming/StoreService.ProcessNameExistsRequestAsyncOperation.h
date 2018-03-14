// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessNameExistsRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessNameExistsRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        
        DEFINE_PERF_COUNTERS ( NameExists )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);  

    private:
        Common::ErrorCode GetNonHierarchyName(__out NameData &);
    };
}
