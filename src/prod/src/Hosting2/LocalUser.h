// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class LocalUser : public ISecurityPrincipal, public SecurityUser
    {
        DENY_COPY(LocalUser)

    public:
        LocalUser(
            std::wstring const& applicationId, 
            ServiceModel::SecurityUserDescription const & userDescription);
        LocalUser(
             std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            bool loadProfile,
            bool performInteractiveLogon,
            ServiceModel::NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups);

        virtual ~LocalUser();

        __declspec(property(get=get_AccountName)) std::wstring const & AccountName;
        virtual std::wstring const & get_AccountName() 
        {
            ASSERT_IF(accountName_.empty(), "SecurityUser.AccountName called without calling CreateAccount() or LoadAccount()");
            return this->accountName_; 
        }

        __declspec(property(get = get_NTLMAuthenticationPolicy)) ServiceModel::NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicy;
        virtual ServiceModel::NTLMAuthenticationPolicyDescription const & get_NTLMAuthenticationPolicy() { return this->ntlmAuthenticationDesc_; }

		// Creates new account with specified name. Returns an error if account with same name already exists
        virtual Common::ErrorCode CreateAccountwithName(std::wstring const& accountName, std::wstring const& comment);
        // Creates new account with a random name.
        virtual Common::ErrorCode CreateAccountwithRandomName(std::wstring const& comment);
        // Deletes existing account. Fails with NotFound if account not found
        virtual Common::ErrorCode DeleteAccount();

        // Loads an already existing account. Will fail if account not found
        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
        // Only Unloads an account. No deletion 
        virtual void UnloadAccount();

		virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken);

    private:
		static Common::GlobalWString PasswordPrefix;
        static Common::GlobalWString AccountNamePrefix;
        static DWORD const RandomDigitsInPassword;
        static int const RandomAccountNameLength;

        Common::ErrorCode GeneratePasswordForNTLMAuthenticatedUser(
            std::wstring const & accountName, 
            ServiceModel::NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy, 
            __out std::wstring & password);

        static Common::ErrorCode DeleteUserAccount(
            std::wstring const & accountName,
            std::wstring const & stringSid,
            bool ignoreDeleteProfileError);
    };
}
