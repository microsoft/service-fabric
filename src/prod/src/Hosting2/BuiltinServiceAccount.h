// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class BuiltinServiceAccount : public SecurityUser
    {
        DENY_COPY(BuiltinServiceAccount)

    public:
        BuiltinServiceAccount(
            std::wstring const& applicationId, 
            ServiceModel::SecurityUserDescription const & userDescription);
        BuiltinServiceAccount(
            std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            bool loadProfile,
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups);

        virtual ~BuiltinServiceAccount();

        virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken);
        virtual Common::ErrorCode ConfigureAccount();
        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
    private:
        Common::ErrorCode GetProcessHandle(__out Common::ProcessHandleUPtr & procHandle); 
        std::wstring domain_;
        bool configured_;
    };
}
