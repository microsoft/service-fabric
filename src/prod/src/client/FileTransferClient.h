// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FileTransferClient : 
        public Common::RootedObject,
        public Common::FabricComponent,
        public Common::TextTraceComponent<Common::TraceTaskCodes::FileTransfer>
    {
    public:
        FileTransferClient(
            ClientConnectionManagerSPtr const &,
            Common::ComponentRoot const & root);

        Common::AsyncOperationSPtr BeginUploadFile(
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool shouldOverwrite,
            Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent,
            Common::ErrorCode const & passThroughError);
        Common::ErrorCode EndUploadFile(
            Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginDownloadFile(            
            std::wstring const & serviceName,            
            std::wstring const & storeRelativePath,
            std::wstring const & destinationFullPath,
            Management::FileStoreService::StoreFileVersion const & version,
            std::vector<std::wstring> const & availableShares,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent,
            Common::ErrorCode const & passThroughError);
        Common::ErrorCode EndDownloadFile(
            Common::AsyncOperationSPtr const &operation);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        void OnFileSenderMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);
        void OnFileReceiverMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);
        void OnFileStoreClientManagerMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);

        void RegisterMessageHandlers();
        void UnregisterMessageHandlers();

        bool ValidateRequiredHeaders(Transport::Message &);

        class BaseAsyncOperation;
        class UploadAsyncOperation;
        class DownloadAsyncOperation;

    private:
        ClientConnectionManagerSPtr connection_;
        ClientServerTransportSPtr transport_;

        std::unique_ptr<FileSender> fileSender_;
        std::unique_ptr<FileReceiver> fileReceiver_;

        Common::SynchronizedMap<Common::Guid, Common::AsyncOperationSPtr> pendingOperations_;
    };
}
