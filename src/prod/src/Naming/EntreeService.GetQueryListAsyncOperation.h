// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::GetQueryListAsyncOperation : public Common::TimedAsyncOperation
    {
    public:
        GetQueryListAsyncOperation(            
            Query::QueryGatewayUPtr & queryGateway,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        static Common::ErrorCode End(Common::AsyncOperationSPtr operation, Transport::MessageUPtr & reply);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);                
        
    private:
        Query::QueryGatewayUPtr & queryGateway_;
        Transport::MessageUPtr reply_;
    };
}
