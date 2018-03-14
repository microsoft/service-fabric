// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class ISecurityPrincipal : protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:
        __declspec(property(get=get_AccountName)) std::wstring const & AccountName;
        virtual std::wstring const & get_AccountName() = 0;

        __declspec(property(get = get_NTLMAuthenticationPolicy)) ServiceModel::NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicy;
        virtual ServiceModel::NTLMAuthenticationPolicyDescription const & get_NTLMAuthenticationPolicy() = 0;

        // Creates new account with specified name. Returns an error if account with same name already exists
        virtual Common::ErrorCode CreateAccountwithName(std::wstring const& accountName, std::wstring const& comment) = 0;
        // Creates new account with a random name.
        virtual Common::ErrorCode CreateAccountwithRandomName(std::wstring const& comment) = 0;
        // Deletes existing account. Fails with NotFound if account not found
        virtual Common::ErrorCode DeleteAccount() = 0;

        // Loads an already existing account. Will fail if account not found
        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName) = 0;
        // Only Unloads an account. No deletion 
        virtual void UnloadAccount() = 0;
    };
}
