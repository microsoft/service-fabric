// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    typedef ComPointer<IFabricTransportMessage> ComFabricTransportMessageCPtr;

    class ComProxyFabricTransportMessageHandler :
        public Common::ComponentRoot,
        public IServiceCommunicationMessageHandler
    {
        DENY_COPY(ComProxyFabricTransportMessageHandler);

    public:
        ComProxyFabricTransportMessageHandler(Common::ComPointer<IFabricTransportMessageHandler> const & comImpl,
            Common::ComPointer<IFabricTransportMessageDisposer> const & comDisposerImpl);
        virtual ~ComProxyFabricTransportMessageHandler();

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            std::wstring const & clientId,
            Transport::MessageUPtr && message,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &  operation,
            Transport::MessageUPtr & reply);

        virtual Common::ErrorCode HandleOneWay(
            std::wstring const & clientId,
            Transport::MessageUPtr && message);

        virtual void Initialize();

        virtual void Release()
        {
        }

    private:
        class ProcessRequestAsyncOperation;
        std::unique_ptr<Common::BatchJobQueue<ComFabricTransportMessageCPtr, Common::ComponentRoot>> disposeQueue_;
        Common::ComPointer<IFabricTransportMessageHandler>  comImpl_;
        Common::ComPointer<IFabricTransportMessageDisposer>  comDisposerImpl_;
    };
}
