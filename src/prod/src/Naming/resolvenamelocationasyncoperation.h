// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolveNameLocationAsyncOperation 
        : public Common::AsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingCommon>
    {
    public:
        ResolveNameLocationAsyncOperation(
            __in Reliability::ServiceResolver & serviceResolver,
            Common::NamingUri const & toResolve,
            Reliability::CacheMode::Enum refreshMode,
            ServiceLocationVersion const & previousVersionUsed,
            NamingServiceCuidCollection const & namingServiceCuids,
            std::wstring const & baseTraceId,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operationRoot);

        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out SystemServices::SystemServiceLocation & locationResult,
            __out ServiceLocationVersion & partitionLocationVersionUsed);

    private:
        void OnResolveServiceComplete(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);

        Reliability::ServiceResolver & serviceResolver_;
        Common::NamingUri toResolve_;
        Reliability::CacheMode::Enum refreshMode_;
        ServiceLocationVersion previousVersionUsed_;
        NamingServiceCuidCollection const & namingServiceCuids_;
        Common::TimeSpan timeout_;
        SystemServices::SystemServiceLocation serviceLocationResult_;
    };
}
