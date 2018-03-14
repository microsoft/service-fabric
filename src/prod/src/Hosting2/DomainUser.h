// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class DomainUser : public SecurityUser
    {
        DENY_COPY(DomainUser)

    public:
        DomainUser(
            std::wstring const& applicationId, 
            ServiceModel::SecurityUserDescription const & userDescription);

        DomainUser(
            std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            bool loadProfile,
            bool performInteractiveLogon,
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups);

        virtual ~DomainUser();

        __declspec(property(get=get_IsPasswordEncrypted)) bool const IsPasswordEncrypted;
        bool const get_IsPasswordEncrypted() const { return this->isPasswordEncrypted_; }

        virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken);

        Common::ErrorCode UpdateCachedCredentials();

    private:

        std::wstring domain_;
        std::wstring username_;
    };
}
