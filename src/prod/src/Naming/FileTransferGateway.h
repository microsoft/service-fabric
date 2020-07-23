// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "client/ClientConnectionManager.h"  
#include "client/FileSender.h"
#include "client/FileReceiver.h" 

namespace Naming
{
    class FileTransferGateway 
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>
    {
    public:
        FileTransferGateway(
            Federation::NodeInstance const &instance,
            Transport::IDatagramTransportSPtr &transport,
            Transport::DemuxerUPtr &demuxer,
            Common::ComponentRoot const & root);

        __declspec (property(get=get_Instance)) Federation::NodeInstance & Instance;
        Federation::NodeInstance & get_Instance() { return instance_; }

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        void OnFileSenderMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);
        void OnFileReceiverMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);
        void OnFileStoreGatewayManagerMessageReceived(Transport::MessageUPtr && message, Transport::ReceiverContextUPtr && receiveContext);

        void RegisterMessageHandlers();
        void UnregisterMessageHandlers();

        bool ValidateRequiredHeaders(Transport::Message &);

        bool AccessCheck(Transport::Message & mesasge);

        void ProcessFileChunkUploadRequest(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            bool const shouldOverwrite,
            uint64 const fileSize,
            Common::Guid const & operationId,
            Transport::ISendTarget::SPtr const & target,
            Common::TimeSpan const & timeout);
        void ProcessFileUploadRequest(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            bool const shouldOverwrite,
            Common::Guid const & operationId,
            Transport::ISendTarget::SPtr const & target,
            Common::TimeSpan const & timeout);
        void ProcessFileDownloadRequest(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            Management::FileStoreService::StoreFileVersion const & storeFileVersion,
            std::vector<std::wstring> && availableShares,
            Common::Guid const & operationId,
            Transport::ISendTarget::SPtr const & target,
            Common::TimeSpan const & timeout);

        void SendResponseToClient(Common::ErrorCode const & error, Transport::ISendTarget::SPtr target, Transport::Actor::Enum const actor, Common::Guid const operationId_, bool useSingleFileUploadProtocolVersion = false);
        Common::ErrorCode ReplyToSender(Transport::ISendTarget::SPtr const & sendTarget, Common::Guid const & operationId, uint64 const sequenceId, Common::ErrorCode const & error, Common::TimeSpan timeout);

        void CleanupUploadChunks();
        bool IsNativeImageStoreEnabled() const;

        class UploadChunkAsyncOperation;
        class UploadAsyncOperation;
        class DownloadAsyncOperation;

    private:
        std::wstring workingDir_;
        bool nativeImageStoreEnabled_;

        std::unique_ptr<Client::FileSender> fileSender_;
        std::unique_ptr<Client::FileReceiver> fileReceiver_;

        Api::IClientFactoryPtr clientFactorPtr_;
        std::shared_ptr<Management::FileStoreService::IFileStoreClient> fileStoreClient_;
        Api::IInternalFileStoreServiceClientPtr fileStoreServiceClient_;
        Client::ClientServerTransportSPtr clientServerTransport_;

        Transport::IDatagramTransportSPtr &transport_;
        Transport::DemuxerUPtr &demuxer_;
        Federation::NodeInstance instance_;
        Common::SynchronizedMap<Common::Guid, Common::AsyncOperationSPtr> pendingOperations_;
        Common::Guid fssPartitionGuid_;
    };
}
