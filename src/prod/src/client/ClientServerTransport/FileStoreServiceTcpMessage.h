// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class FileStoreServiceTcpMessage : public Client::ClientServerRequestMessage
    {

    public:

        // Request actions
        static Common::GlobalWString GetStagingLocationAction;
        static Common::GlobalWString GetStoreLocationAction;
        static Common::GlobalWString GetStoreLocationsAction;
        static Common::GlobalWString UploadAction;
        static Common::GlobalWString UploadChunkContentAction;
        static Common::GlobalWString CopyAction;
        static Common::GlobalWString DeleteAction;
        static Common::GlobalWString CheckExistenceAction;
        static Common::GlobalWString ListAction;
        static Common::GlobalWString InternalListAction;
        static Common::GlobalWString ListUploadSessionAction;
        static Common::GlobalWString CreateUploadSessionAction;
        static Common::GlobalWString UploadChunkAction;
        static Common::GlobalWString DeleteUploadSessionAction;
        static Common::GlobalWString CommitUploadSessionAction;

        FileStoreServiceTcpMessage(std::wstring const & action)
            : Client::ClientServerRequestMessage(action, actor_)
        {
            WrapForFileStoreService();
        }

        FileStoreServiceTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body))
        {
            WrapForFileStoreService();
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetStoreLocation()
        { 
            return Common::make_unique<FileStoreServiceTcpMessage>(GetStoreLocationAction);
        } 

        static Client::ClientServerRequestMessageUPtr GetStoreLocations()
        { 
            return Common::make_unique<FileStoreServiceTcpMessage>(GetStoreLocationsAction);
        } 

        static Client::ClientServerRequestMessageUPtr GetStagingLocation()
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(GetStagingLocationAction);
        }             

        static Client::ClientServerRequestMessageUPtr GetUpload(std::unique_ptr<Management::FileStoreService::UploadRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(UploadAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCopy(std::unique_ptr<Management::FileStoreService::CopyRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(CopyAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDelete(std::unique_ptr<Management::FileStoreService::ImageStoreBaseRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(DeleteAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCheckExistence(std::unique_ptr<Management::FileStoreService::ImageStoreBaseRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(CheckExistenceAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetList(std::unique_ptr<Management::FileStoreService::ListRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(ListAction, std::move(body));
        }            

        static Client::ClientServerRequestMessageUPtr GetInternalList(std::unique_ptr<Management::FileStoreService::ListRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(InternalListAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetListUploadSession(std::unique_ptr<Management::FileStoreService::UploadSessionRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(ListUploadSessionAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCreateUploadSession(std::unique_ptr<Management::FileStoreService::CreateUploadSessionRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(CreateUploadSessionAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteUploadSession(std::unique_ptr<Management::FileStoreService::DeleteUploadSessionRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(DeleteUploadSessionAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUploadChunk(std::unique_ptr<Management::FileStoreService::UploadChunkRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(UploadChunkAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetUploadChunkContent(std::unique_ptr<Management::FileStoreService::UploadChunkContentRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(UploadChunkContentAction, std::move(body));
        }

        static Client::ClientServerRequestMessageUPtr GetCommitUploadSession(std::unique_ptr<Management::FileStoreService::UploadSessionRequest> && body)
        {
            return Common::make_unique<FileStoreServiceTcpMessage>(CommitUploadSessionAction, std::move(body));
        }

    private:

        void WrapForFileStoreService()
        {
            SystemServices::ServiceRoutingAgentMessage::WrapForRoutingAgent(
                *this,
                *ServiceModel::ServiceTypeIdentifier::FileStoreServiceTypeId);
        }

        static const Transport::Actor::Enum actor_;
    };
}
