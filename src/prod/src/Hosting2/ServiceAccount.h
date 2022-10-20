// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class ServiceAccount : public SecurityUser
    {
        DENY_COPY(ServiceAccount)

    public:
        ServiceAccount(
            std::wstring const& applicationId, 
            ServiceModel::SecurityUserDescription const & userDescription);
        ServiceAccount(
            std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            bool loadProfile,
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups);

        virtual ~ServiceAccount();

        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
        virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken);

    private:

        //Tries to import account in case this is a managed service account
        //or group managed service account
        void SetupServiceAccount();

        std::wstring domain_;
        std::wstring serviceAccountName_;

    };
}
