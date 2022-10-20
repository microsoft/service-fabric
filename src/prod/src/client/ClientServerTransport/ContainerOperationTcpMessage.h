// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class ContainerOperationTcpMessage : public Client::ClientServerRequestMessage
    {
    public:
        static Common::GlobalWString CreateComposeDeploymentAction;
        static Common::GlobalWString DeleteComposeDeploymentAction;
        static Common::GlobalWString UpgradeComposeDeploymentAction;
        static Common::GlobalWString RollbackComposeDeploymentUpgradeAction;
        static Common::GlobalWString DeleteSingleInstanceDeploymentAction;

        ContainerOperationTcpMessage(
            std::wstring const &action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body), activityId)
            , composeFiles_()
            , sfSettingsFiles_()
        {
        }

        //
        // File data is sent via the const buffers in the transport message. Other details
        // about the operation are sent as headers.
        //
        ContainerOperationTcpMessage(
            std::wstring const &action,
            std::vector<Common::ByteBuffer> && composeFiles,
            std::vector<Common::ByteBuffer> && sfSettingsFiles,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, actor_, activityId)
            , composeFiles_()
            , sfSettingsFiles_()
        {
            composeFiles_ = make_unique<std::vector<Common::ByteBuffer>>();
            sfSettingsFiles_ = make_unique<std::vector<Common::ByteBuffer>>();

            for (auto it = composeFiles.begin(); it != composeFiles.end(); ++it)
            {
                composeFiles_->push_back(std::move(*it));
            }

            for (auto it = sfSettingsFiles.begin(); it != sfSettingsFiles.end(); ++it)
            {
                sfSettingsFiles_->push_back(std::move(*it));
            }
        }

        static Client::ClientServerRequestMessageUPtr GetCreateComposeDeploymentMessage(
            std::wstring const &deploymentName,
            std::wstring const &applicationName,
            std::vector<Common::ByteBuffer> && composeFiles,
            std::vector<Common::ByteBuffer> && sfSettingsFiles,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool isPasswordEncrypted,
            Common::ActivityId const &activityId)
        {
            std::vector<size_t> composeFileSizes, sfSettingsFileSizes;

            GetFileSizes(composeFiles, sfSettingsFiles, composeFileSizes, sfSettingsFileSizes);

            auto message = Common::make_unique<ContainerOperationTcpMessage>(CreateComposeDeploymentAction, move(composeFiles), move(sfSettingsFiles), activityId);
            message->Headers.Add(CreateComposeDeploymentRequestHeader(deploymentName, applicationName, move(composeFileSizes), move(sfSettingsFileSizes), repositoryUserName, repositoryPassword, isPasswordEncrypted));
            return std::move(message);
        }

        static Client::ClientServerRequestMessageUPtr GetUpgradeComposeDeploymentMessage(
            ServiceModel::ComposeDeploymentUpgradeDescription const &description,
            Common::ActivityId const &activityId)
        {
            std::vector<size_t> composeFileSizes, sfSettingsFileSizes;
            auto composeFiles = description.TakeComposeFiles();
            auto sfSettingsFiles = description.TakeSettingsFiles();

            GetFileSizes(composeFiles, sfSettingsFiles, composeFileSizes, sfSettingsFileSizes);

            auto message = Common::make_unique<ContainerOperationTcpMessage>(UpgradeComposeDeploymentAction, move(composeFiles), move(sfSettingsFiles), activityId);
            message->Headers.Add(UpgradeComposeDeploymentRequestHeader(move(composeFileSizes), move(sfSettingsFileSizes), description));
            return std::move(message);
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteContainerApplicationMessage(
            std::wstring const & deploymentName,
            Common::NamingUri const &applicationName,
            Common::ActivityId const &activityId)
        {
            auto message = Common::make_unique<ContainerOperationTcpMessage>(
                DeleteComposeDeploymentAction,
                Common::make_unique<Management::ClusterManager::DeleteComposeDeploymentMessageBody>(deploymentName, applicationName),
                activityId);

            return std::move(message);
        }

        static Client::ClientServerRequestMessageUPtr GetRollbackComposeDeploymentMessage(
            std::wstring const & deploymentName,
            Common::ActivityId const &activityId)
        {
            auto message = Common::make_unique<ContainerOperationTcpMessage>(
                RollbackComposeDeploymentUpgradeAction,
                Common::make_unique<Management::ClusterManager::RollbackComposeDeploymentMessageBody>(deploymentName),
                activityId);

            return std::move(message);
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteSingleInstanceDeploymentMessage(
            ServiceModel::DeleteSingleInstanceDeploymentDescription const & description,
            Common::ActivityId const & activityId)
        {
            auto message = Common::make_unique<ContainerOperationTcpMessage>(
                DeleteSingleInstanceDeploymentAction,
                Common::make_unique<Management::ClusterManager::DeleteSingleInstanceDeploymentMessageBody>(description),
                activityId);

            return std::move(message);
        }

        Transport::MessageUPtr GetTcpMessage() override
        {
            Transport::MessageUPtr msg;
            if (composeFiles_ != nullptr && !composeFiles_->empty())
            {
                std::vector<Common::const_buffer> constBuffers;

                // first - the compose files.
                for (auto it = this->composeFiles_->begin(); it != this->composeFiles_->end(); ++it)
                {
                    constBuffers.push_back(Common::const_buffer((*it).data(), (*it).size()));
                }

                // followed by sf settings file
                for (auto it = this->sfSettingsFiles_->begin(); it != this->sfSettingsFiles_->end(); ++it)
                {
                    constBuffers.push_back(Common::const_buffer((*it).data(), (*it).size()));
                }

                Common::MoveUPtr<std::vector<Common::ByteBuffer>> composeMover(std::move(this->composeFiles_));
                Common::MoveUPtr<std::vector<Common::ByteBuffer>> sfSettingsFileMover(std::move(this->sfSettingsFiles_));
                msg = Common::make_unique<Transport::Message>(
                    constBuffers,
                    [composeMover, sfSettingsFileMover](std::vector<Common::const_buffer> const &, void *) 
                    { 
                    },
                    nullptr);
            }
            else
            {
                msg = Client::ClientServerRequestMessage::CreateTcpMessage();
            }

            msg->Headers.AppendFrom(this->Headers);
            return std::move(msg);
        }

    private:

        static void GetFileSizes(
            std::vector<Common::ByteBuffer> const & composeFiles,
            std::vector<Common::ByteBuffer> const & sfSettingsFiles,
            __out std::vector<size_t> &composeFileSizes,
            __out std::vector<size_t> &sfSettingsFileSizes)
        {
            composeFileSizes.reserve(composeFiles.size());
            for (auto it = composeFiles.begin(); it != composeFiles.end(); ++it)
            {
                composeFileSizes.push_back((*it).size());
            }

            sfSettingsFileSizes.reserve(sfSettingsFiles.size());
            for (auto it = sfSettingsFiles.begin(); it != sfSettingsFiles.end(); ++it)
            {
                sfSettingsFileSizes.push_back((*it).size());
            }
        }

        static const Transport::Actor::Enum actor_;
        std::unique_ptr<std::vector<Common::ByteBuffer>> composeFiles_;
        std::unique_ptr<std::vector<Common::ByteBuffer>> sfSettingsFiles_;

    };
}

