// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::CompletedRequestAsyncOperation : public RequestAsyncOperationBase
    {
    public:
        CompletedRequestAsyncOperation(
            __in GatewayProperties & properties,
            Transport::FabricActivityHeader const & activityHeader,
            Common::ErrorCode const & error,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
            : RequestAsyncOperationBase(
                properties,
                Common::NamingUri::RootNamingUri,
                activityHeader,
                Common::TimeSpan::MaxValue,
                callback,
                parent)
            , error_(error)
        {
        }

        ~CompletedRequestAsyncOperation() { }

    protected:

        virtual void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr)
        {
            this->TryComplete(thisSPtr, error_);
        }

    private:

        Common::ErrorCode error_;
    };
}
