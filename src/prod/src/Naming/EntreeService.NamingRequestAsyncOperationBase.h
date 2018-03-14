// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::NamingRequestAsyncOperationBase : public EntreeService::RequestAsyncOperationBase
    {
    public:
        NamingRequestAsyncOperationBase(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        NamingRequestAsyncOperationBase(
            __in GatewayProperties & properties,
            Common::NamingUri const & name,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:

        virtual void OnRetry(Common::AsyncOperationSPtr const & thisSPtr) = 0;    

        virtual void OnStoreCommunicationFinished(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr && storeReply) = 0;    

        virtual Transport::MessageUPtr CreateMessageForStoreService() = 0;

        virtual void OnResolveNamingServiceRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Common::ErrorCode const &);

        virtual void OnResolveNamingServiceNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Common::ErrorCode const &);
        
        virtual void OnRouteToNodeFailedRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply,
            Common::ErrorCode const & error);

        virtual void OnNameNotFound(Common::AsyncOperationSPtr const & thisSPtr);

        void StartStoreCommunication(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Common::NamingUri const & toResolve);

    private:

        void OnResolveNamingServiceComplete(
            Common::AsyncOperationSPtr const & asyncOperation, 
            bool expectedCompletedSynchronously);

        virtual void OnRouteToNodeSuccessful(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply);

        virtual void OnRouteToNodeFailedNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error);

        void RaiseResolutionErrorAndComplete(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error);

        ServiceLocationVersion lastNamingPartitionLocationVersionUsed_;
    };
}
