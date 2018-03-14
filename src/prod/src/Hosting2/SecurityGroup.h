// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SecurityGroup : public ISecurityPrincipal
    {
        DENY_COPY(SecurityGroup)

    public:
        SecurityGroup(
            std::wstring const & applicationId, 
            ServiceModel::SecurityGroupDescription const & groupDescription);
        virtual ~SecurityGroup();

        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return this->groupDescription_.Name; }

        __declspec(property(get=get_AccountName)) std::wstring const & AccountName;
        virtual std::wstring const & get_AccountName() 
        {
            ASSERT_IF(groupName_.empty(), "SecurityGroup.GroupName called without calling CreateGroup()");
            return this->groupName_; 
        }

        __declspec(property(get = get_NTLMAuthenticationPolicy)) ServiceModel::NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicy;
        virtual ServiceModel::NTLMAuthenticationPolicyDescription const & get_NTLMAuthenticationPolicy() 
        { 
            return this->groupDescription_.NTLMAuthenticationPolicy; 
        }

        __declspec(property(get=get_Sid)) Common::SidUPtr const & SidObj;
        Common::SidUPtr const & get_Sid() const { return this->sid_; }

        virtual Common::ErrorCode CreateAccountwithName(
            std::wstring const& accountName, 
            std::wstring const& comment);
        virtual Common::ErrorCode CreateAccountwithRandomName(std::wstring const& comment);
        virtual Common::ErrorCode DeleteAccount();

        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
        virtual void UnloadAccount();

        // Get the account Sid;
        // The sid only exists after the account is successfully created;
        // if not set, null is returned
        PSID const TryGetSid() const;

    private:
        static int const GroupNameLength;
        static Common::GlobalWString GroupNamePrefix;

        Common::ErrorCode SetupMembershipForNewGroup();

        ServiceModel::SecurityGroupDescription groupDescription_;

        // The application id of the application that has requested the creation of the principal
        std::wstring const applicationId_;

        // The name of the group, which is the name specified in the manifest file prefixed by the application Id
        std::wstring groupName_;

        // The security identifier of the account
        Common::SidUPtr sid_;
    };
}
