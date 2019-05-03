// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ClientServerTransport
{
    class CentralSecretServiceMessage;
    typedef std::unique_ptr<CentralSecretServiceMessage> CentralSecretServiceMessageUPtr;

    class CentralSecretServiceMessage : public Client::ClientServerRequestMessage
    {
    public:

        static Common::GlobalWString GetSecretsAction;
        static Common::GlobalWString SetSecretsAction;
        static Common::GlobalWString RemoveSecretsAction;
        static Common::GlobalWString GetSecretVersionsAction;

        CentralSecretServiceMessage(
            std::wstring const & action,
            Common::ActivityId const & activityId);

        CentralSecretServiceMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body);

        CentralSecretServiceMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId);

        static CentralSecretServiceMessageUPtr CreateRequestMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body);

        static CentralSecretServiceMessageUPtr CreateRequestMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
            Common::ActivityId const & activityId);

    private:
        CentralSecretServiceMessage();
    };
}
