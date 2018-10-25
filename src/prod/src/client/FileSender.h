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
            Common::ComponentRoot const & componentRoot,
            bool const useChunkBasedUpload = false);

        static std::unique_ptr<FileSender> CreateOnGateway(
            ClientServerTransportSPtr const &,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot,
            bool const useChunkBasedUpload = false);

    public:
        FileSender(
            ClientConnectionManagerSPtr const & connection,
            bool const includeClientVersionHeaderInMessage,
            wstring const & traceId,
            Common::ComponentRoot const & root,
            bool const useChunkBasedUpload);

        FileSender(
            ClientServerTransportSPtr const &,
            bool const includeClientVersionHeaderInMessage,
            std::wstring const & traceId,
            Common::ComponentRoot const & componentRoot,
            bool const useChunkBasedUpload);

        Common::AsyncOperationSPtr BeginSendFileToGateway(
            Common::Guid const & operationId,
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool const shouldOverwrite,
            bool const useChunkBasedUpload,
            bool const retryAttempt,
            Api::IFileStoreServiceClientProgressEventHandlerPtr &&,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndSendFileToGateway(
            Common::AsyncOperationSPtr const &operation,
            Common::Guid const& operationId, 
            __out bool &isUsingChunkBasedUpload);

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

        __declspec(property(get = get_UseChunkBasedUpload, put = set_UseChunkBasedUpload)) bool UseChunkBasedUpload;
        bool get_UseChunkBasedUpload() const { return useChunkBasedUpload_.load(); }
        void set_UseChunkBasedUpload(bool value) { useChunkBasedUpload_.store(value); }

        __declspec(property(get = get_AtleastOneCreateChunkResponse, put = set_AtleastOneCreateChunkResponse)) bool AtleastOneCreateChunkResponse;
        bool get_AtleastOneCreateChunkResponse() const { return atleastOneCreateChunkResponse_.load(); }
        void set_AtleastOneCreateChunkResponse(bool value) { atleastOneCreateChunkResponse_.store(value); }

        // This method is called for each of the file send completion.
        // When it is called with true consecutively until a threshold is met, we switch the protocol of the file sender to single file upload protocol for all files.
        // When it is called with false, we reset the internal counter.
        // The idea is to avoid just one spurious non-response for create file message shouldn't switch protocol for all files.
        bool SwitchToSingleFileUploadProtocol(bool value);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class SendAsyncOperation;
        class SendChunkBasedAsyncOperation;
        class FileJobItem;
        class FileChunkJobItem;
        typedef std::function<void()> ProcessingCallback;

    private:
        Common::ErrorCode EndSendFile(Common::AsyncOperationSPtr const &operation);
        Common::ErrorCode SendMessage(
            Transport::ISendTarget::SPtr const &sendTarget,
            __out unique_ptr<Naming::GatewayDescription> &currentGateway,
            __out Client::ClientServerRequestMessageUPtr & message,
            Common::TimeSpan timeout);

        bool const includeClientVersionHeaderInMessage_;
        std::wstring const traceId_;

        ClientConnectionManagerSPtr connection_;
        ClientServerTransportSPtr transport_;
        Common::atomic_bool useChunkBasedUpload_;
        Common::atomic_long connectionToClusterFailureCount_;

        std::unique_ptr<Common::CommonJobQueue<FileSender>> jobQueue_;
        Common::SynchronizedMap<Common::Guid, Common::AsyncOperationSPtr> activeSends_;
        Common::SynchronizedMap<Common::Guid, bool> usingFileChunkUploads_;

        Common::atomic_bool atleastOneCreateChunkResponse_;
    };
}
