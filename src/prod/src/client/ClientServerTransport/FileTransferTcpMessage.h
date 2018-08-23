// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class FileTransferTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString FileContentAction;
        static Common::GlobalWString FileDownloadAction;
        static Common::GlobalWString FileCreateAction;
        static Common::GlobalWString FileUploadCommitAction;
        static Common::GlobalWString FileUploadCommitAckAction;
        static Common::GlobalWString FileUploadDeleteSessionAction;

        // Reply action
        static Common::GlobalWString ClientOperationSuccessAction;
        static Common::GlobalWString ClientOperationFailureAction;
        
        FileTransferTcpMessage(
            std::wstring const & action,
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
            : Client::ClientServerRequestMessage(action, actor, Common::ActivityId(operationId)),
            sequenceNumber_(static_cast<UINT>(-1)),
            isLast_(false)
        {
        }

        FileTransferTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body,
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
            : Client::ClientServerRequestMessage(action, actor, std::move(body), Common::ActivityId(operationId)),
            sequenceNumber_(static_cast<UINT>(-1)),
            isLast_(false)
        {
        }

        FileTransferTcpMessage(
            std::wstring const & action,
            std::shared_ptr<std::vector<BYTE>> const & buffer,
            uint64 const sequenceNumber,
            bool const isLast,
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
            : Client::ClientServerRequestMessage(action, actor, Common::ActivityId(operationId)),
            buffer_(buffer),
            sequenceNumber_(sequenceNumber),
            isLast_(isLast)
        {
            Headers.Add(Naming::FileSequenceHeader(sequenceNumber_, isLast_, buffer_->size()));
        }

        Transport::MessageUPtr GetTcpMessage() override
        {
            Transport::MessageUPtr msg;

            if (buffer_)
            {

                std::vector<Common::const_buffer> constBuffers;
                constBuffers.push_back(Common::const_buffer(buffer_->data(), buffer_->size()));

                void * state = nullptr;

                std::shared_ptr<std::vector<BYTE>> buffer = this->buffer_;

                msg = Common::make_unique<Transport::Message>(
                    constBuffers,
                    [buffer] (std::vector<Common::const_buffer> const &, void *) {},
                    state);
            }
            else
            {
                msg = Client::ClientServerRequestMessage::CreateTcpMessage();
            }

            msg->Headers.AppendFrom(this->Headers);

            return std::move(msg);
        }

        // Requests
        static Client::ClientServerRequestMessageUPtr GetFileContentMessage(
            std::shared_ptr<std::vector<BYTE>> const & buffer,
            uint64 const sequenceNumber,
            bool const isLast,             
            Common::Guid const & operationId, 
            Transport::Actor::Enum actor)
        { 
            return Common::make_unique<FileTransferTcpMessage>(FileContentAction, buffer, sequenceNumber, isLast, operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetFileCreateMessage(
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            return Common::make_unique<FileTransferTcpMessage>(FileCreateAction, operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetFileUploadCommitMessage(
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            return Common::make_unique<FileTransferTcpMessage>(FileUploadCommitAction, operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetFileUploadCommitAckMessage(
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            return Common::make_unique<FileTransferTcpMessage>(FileUploadCommitAckAction, operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetFileUploadDeleteSessionMessage(
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            return Common::make_unique<FileTransferTcpMessage>(FileUploadDeleteSessionAction, operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetFileDownloadMessage(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            Management::FileStoreService::StoreFileVersion const & storeFileVersion,
            std::vector<std::wstring> const & availableShares,
            Common::Guid const & operationId, 
            Transport::Actor::Enum actor)
        { 
            return Common::make_unique<FileTransferTcpMessage>(FileDownloadAction, Common::make_unique<Naming::FileDownloadMessageBody>(serviceName, storeRelativePath, storeFileVersion, availableShares), operationId, actor);
        }    

        static Client::ClientServerRequestMessageUPtr GetFailureMessage(
            Common::ErrorCode const & error,
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            Common::ErrorCode localCopy = error;
            return Common::make_unique<FileTransferTcpMessage>(ClientOperationFailureAction, Common::make_unique<Naming::ClientOperationFailureMessageBody>(std::move(localCopy)), operationId, actor);
        }

        static Client::ClientServerRequestMessageUPtr GetSuccessMessage(
            Common::Guid const & operationId,
            Transport::Actor::Enum actor)
        {
            return Common::make_unique<FileTransferTcpMessage>(ClientOperationSuccessAction, operationId, actor);
        }

        static bool TryGetErrorOnClientOperationFailureAction(
            Transport::Message & message,
            __out Common::ErrorCode & error)
        {
            Naming::ClientOperationFailureMessageBody body;
            if (message.GetBody(body))
            {
                error = body.TakeError();
                return true;
            }
            else
            {
                return false;
            }
        }

    private:

        std::shared_ptr<std::vector<BYTE>> const buffer_;
        uint64 const sequenceNumber_;
        bool const isLast_;
    };
}
