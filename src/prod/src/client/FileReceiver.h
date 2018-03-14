// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{    
    class FileReceiver : 
        public Common::RootedObject,
        public Common::FabricComponent,
        public Common::TextTraceComponent<Common::TraceTaskCodes::FileTransfer>
    {
    public:
        FileReceiver(
            ClientServerTransportSPtr const &transport,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & workingDir,
            std::wstring const & traceId,
            Common::ComponentRoot const & root);

        Common::AsyncOperationSPtr BeginReceiveFile(
            Common::Guid const & operationId,
            std::wstring const & destinationFullPath,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndReceiveFile(Common::AsyncOperationSPtr const &operation);

        void ProcessMessage(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiverContext);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        Common::ErrorCode ReplyToSender(
            Transport::ISendTarget::SPtr const & sendTarget, 
            Common::Guid const & operationId, 
            Common::ErrorCode const & error,
            Common::TimeSpan timeout = Common::TimeSpan::MaxValue);

        class ReceiveAsyncOperation;        
    private:
        bool const includeClientVersionHeaderInMessage_;
        std::wstring const workingDir_;
        std::wstring const traceId_;

        ClientServerTransportSPtr transport_;
        Common::SynchronizedMap<Common::Guid, Common::AsyncOperationSPtr> activeDownloads_;
    };
}
