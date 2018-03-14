// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class RepositoryAuthConfig
        : public Common::IFabricJsonSerializable
    {
    public:
        RepositoryAuthConfig();

        ~RepositoryAuthConfig();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(RepositoryAuthConfig::AccountNameParameter, AccountName)
          SERIALIZABLE_PROPERTY(RepositoryAuthConfig::PasswordParameter, Password)
          SERIALIZABLE_PROPERTY_IF(RepositoryAuthConfig::EmailParameter, Email, !Email.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring AccountName;
		std::wstring Password;
		std::wstring Email;

        static Common::WStringLiteral const AccountNameParameter;
        static Common::WStringLiteral const PasswordParameter;
        static Common::WStringLiteral const EmailParameter;
    };
}
