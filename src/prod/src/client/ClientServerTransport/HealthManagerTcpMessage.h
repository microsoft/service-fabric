// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class HealthManagerTcpMessage : public Client::ClientServerRequestMessage
    {
    public:

        // Request actions
        static Common::GlobalWString ReportHealthAction;
        static Common::GlobalWString GetSequenceStreamProgressAction;

        HealthManagerTcpMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, actor_, std::move(body), activityId)
        {
        }

        // Requests 
        static Client::ClientServerRequestMessageUPtr GetReportHealth(
            Common::ActivityId const & activityId,
            std::unique_ptr<Management::HealthManager::ReportHealthMessageBody> && body) 
        {
            return Common::make_unique<HealthManagerTcpMessage>(ReportHealthAction, std::move(body), activityId);
        }

        static Client::ClientServerRequestMessageUPtr GetSequenceStreamProgressRequest(
            std::unique_ptr<Management::HealthManager::GetSequenceStreamProgressMessageBody> && body,
            Common::ActivityId const & activityId) 
        {
            return Common::make_unique<HealthManagerTcpMessage>(GetSequenceStreamProgressAction, std::move(body), activityId);
        }

    private:

        static const Transport::Actor::Enum actor_;
    };
}
