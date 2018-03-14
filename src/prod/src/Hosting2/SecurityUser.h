// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class SecurityUser : protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(SecurityUser)

    public:
        static SecurityUserSPtr CreateSecurityUser(
            std::wstring const& applicationId, 
            ServiceModel::SecurityUserDescription const & userDescription);

         static SecurityUserSPtr CreateSecurityUser(std::wstring const& applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            bool loadProfile,
            bool performInteractiveLogon,
            ServiceModel::NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups = std::vector<std::wstring>(),
            std::vector<std::wstring> parentSystemGroups = std::vector<std::wstring>());

        SecurityUser(
            std::wstring const & applicationId,
            ServiceModel::SecurityUserDescription const & userDescription);

        SecurityUser(
            std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,            
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups,
            bool loadProfile = false,
            bool performInteractiveLogon = false);

        SecurityUser(
            std::wstring const & applicationId,
            std::wstring const & name,
            std::wstring const & accountName,
            std::wstring const & password,
            bool isPasswordEncrypted,
            ServiceModel::NTLMAuthenticationPolicyDescription const & ntlmAuthenticationPolicy,
            ServiceModel::SecurityPrincipalAccountType::Enum accountType,
            std::vector<std::wstring> parentApplicationGroups,
            std::vector<std::wstring> parentSystemGroups,
            bool loadProfile = false,
            bool performInteractiveLogon = false);


        virtual ~SecurityUser();

        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return this->name_; }

        __declspec(property(get=get_Sid)) Common::SidSPtr const & SidObj;
        Common::SidSPtr const & get_Sid() const { return this->sid_; }

        __declspec(property(get=get_Password)) std::wstring const & Password;
        std::wstring const & get_Password() const { return this->password_; }

        __declspec(property(get=get_PerformInteractiveLogon)) bool const PerformInteractiveLogon;
        bool const get_PerformInteractiveLogon() const { return this->performInteractiveLogon_; }

        __declspec(property(get=get_LoadProfile)) bool const LoadProfile;
        bool const get_LoadProfile() const { return this->loadProfile_; }

        __declspec(property(get = get_NTLMAuthenticationPolicy)) ServiceModel::NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicy;
        virtual ServiceModel::NTLMAuthenticationPolicyDescription const & get_NTLMAuthenticationPolicy() { return this->ntlmAuthenticationDesc_; }

        __declspec(property(get=get_AccountName)) std::wstring const & AccountName;
        virtual std::wstring const & get_AccountName() 
        {
            ASSERT_IF(accountName_.empty() && AccountType == ServiceModel::SecurityPrincipalAccountType::LocalUser,
                "SecurityUser.AccountName called without calling CreateAccount() or LoadAccount()");
            return this->accountName_; 
        }

        // Loads an already existing account. Will fail if account not found
        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
        // Only Unloads an account. No deletion 
        virtual void UnloadAccount();

        __declspec(property(get=get_AccountType)) ServiceModel::SecurityPrincipalAccountType::Enum const AccountType;
        virtual ServiceModel::SecurityPrincipalAccountType::Enum const get_AccountType() 
        {
            return accountType_;
        }

        virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken) = 0;

        virtual Common::ErrorCode ConfigureAccount();

        bool IsPasswordUpdated(SecurityUserSPtr const & other);

    protected:

        Common::ErrorCode SetupGroupMembershipForNewUser();

        __declspec(property(get=get_SidToAdd)) PSID const SidToAdd;
        PSID const get_SidToAdd() const { return this->sidToAdd_ ? this->sidToAdd_->PSid : NULL; }

        std::wstring name_;
        std::wstring accountName_;
        
        std::wstring password_;
        bool isPasswordEncrypted_;
        bool loadProfile_;
        bool performInteractiveLogon_;
        
        ServiceModel::NTLMAuthenticationPolicyDescription ntlmAuthenticationDesc_;
        
        Common::SidSPtr sidToAdd_; 

        ServiceModel::SecurityPrincipalAccountType::Enum accountType_;

        std::vector<std::wstring> parentApplicationGroups_;
        std::vector<std::wstring> parentSystemGroups_;

        std::wstring applicationId_;

        // The security identifier of the account
        Common::SidSPtr sid_;

        Common::ExclusiveLock userLock_;
        Common::AccessTokenSPtr userToken_;
    };
}
