// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FileSender :
        public Common::RootedObject,
        public Common::FabricComponent,
        public Common::TextTraceComponent < Common::TraceTaskCodes::FileTransfer >
    {
    public:

        static std::unique_ptr<FileSender> CreateOnClient(
            ClientConnectionManagerSPtr const &, 
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot);

        static std::unique_ptr<FileSender> CreateOnGateway(
            ClientServerTransportSPtr const &,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot);

    public:

        FileSender(
            ClientConnectionManagerSPtr const &,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot);

        FileSender(
            ClientServerTransportSPtr const &,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot);

        Common::AsyncOperationSPtr BeginSendFileToGateway(
            Common::Guid const & operationId,
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool const shouldOverwrite,
            Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndSendFileToGateway(Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginSendFileToClient(
            Common::Guid const & operationId,
            Transport::ISendTarget::SPtr const &,
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool const shouldOverwrite,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndSendFileToClient(Common::AsyncOperationSPtr const &operation);

        void ProcessMessage(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiverContext);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class SendAsyncOperation;
        class FileJobItem;
        typedef std::function<void()> ProcessingCallback;

    private:
        Common::ErrorCode EndSendFile(Common::AsyncOperationSPtr const &operation);

        bool const includeClientVersionHeaderInMessage_;
        std::wstring const traceId_;

        ClientConnectionManagerSPtr connection_;
        ClientServerTransportSPtr transport_;

        std::unique_ptr<Common::CommonJobQueue<FileSender>> jobQueue_;
        Common::SynchronizedMap<Common::Guid, Common::AsyncOperationSPtr> activeSends_;
    };
}
