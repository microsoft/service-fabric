// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyFabricTransportCallbackMessageHandler :
        public Common::ComponentRoot,
        public IServiceCommunicationMessageHandler
    {
        DENY_COPY(ComProxyFabricTransportCallbackMessageHandler);

    public:
        ComProxyFabricTransportCallbackMessageHandler(Common::ComPointer<IFabricTransportCallbackMessageHandler> const & comImpl);
        virtual ~ComProxyFabricTransportCallbackMessageHandler();

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

        virtual void Release()
        {
        }

        virtual void Initialize() {
            //no-op 
            //Needed for V2 Stack
        }

    private:
        class ProcessRequestAsyncOperation;
        Common::ComPointer<IFabricTransportCallbackMessageHandler>  comImpl_;
    };
}
