// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::GetApplicationNameAsyncOperation : public Common::TimedAsyncOperation
    {
    public:
        GetApplicationNameAsyncOperation(
            __in GatewayProperties & properties,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        static Common::ErrorCode End(Common::AsyncOperationSPtr operation, Transport::MessageUPtr & reply);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
        
    private:
        void OnGetPsdComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        GatewayProperties & properties_;
        Transport::FabricActivityHeader activityHeader_;
        wstring serviceNameStr_;

        Transport::MessageUPtr reply_;
    };
}
