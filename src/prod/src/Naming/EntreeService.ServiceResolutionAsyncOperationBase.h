// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ServiceResolutionAsyncOperationBase : public EntreeService::NamingRequestAsyncOperationBase
    {
    public:
        ServiceResolutionAsyncOperationBase(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:

        __declspec(property(get=get_RequestBody)) ServiceResolutionMessageBody const & RequestBody;
        ServiceResolutionMessageBody const & get_RequestBody() const { return requestBody_; }

        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

        virtual void GetPsd(Common::AsyncOperationSPtr const & thisSPtr) = 0;

    protected:

        void ResolveLocations(
            Common::AsyncOperationSPtr const & thisSPtr,
            PartitionedServiceDescriptorSPtr const &);

        void OnResolveUserServicesComplete(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);

        void SendReplyAndComplete(
            Common::AsyncOperationSPtr const & thisSPtr,
            Reliability::ServiceTableEntry const & cachedLocation,
            Reliability::GenerationNumber const & generation);

    private:

        Common::ErrorCode GetCurrentUserCuidToResolve(
            ServiceResolutionRequestData const & request,
            __out Reliability::VersionedCuid & previous);

        ServiceResolutionMessageBody requestBody_;
        PartitionedServiceDescriptorSPtr psd_;
    };
}
