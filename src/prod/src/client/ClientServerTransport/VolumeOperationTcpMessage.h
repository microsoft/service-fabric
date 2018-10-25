//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class VolumeOperationTcpMessage : public Client::ClientServerRequestMessage
    {
    public:
        static Common::GlobalWString CreateVolumeAction;
        static Common::GlobalWString DeleteVolumeAction;

        VolumeOperationTcpMessage(
            std::wstring const &action,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, actor_, activityId)
        {
        }

        VolumeOperationTcpMessage(
            std::wstring const &action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body), activityId)
        {
        }

        static Client::ClientServerRequestMessageUPtr GetCreateVolumeMessage(
            Management::ClusterManager::VolumeDescriptionSPtr const & description,
            Common::ActivityId const &activityId)
        {
            auto message = Common::make_unique<VolumeOperationTcpMessage>(
                CreateVolumeAction,
                Common::make_unique<CreateVolumeMessageBody>(description),
                activityId);
            return std::move(message);
        }

        static Client::ClientServerRequestMessageUPtr GetDeleteVolumeMessage(
            std::wstring const &volumeName,
            Common::ActivityId const &activityId)
        {
            auto message = Common::make_unique<VolumeOperationTcpMessage>(
                DeleteVolumeAction,
                Common::make_unique<DeleteVolumeMessageBody>(volumeName),
                activityId);
            return std::move(message);
        }

    private:
        static const Transport::Actor::Enum actor_;
    };
}

