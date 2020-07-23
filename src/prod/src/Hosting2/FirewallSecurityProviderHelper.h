// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "atlcomcli.h"

namespace Hosting2
{
    class FirewallSecurityProviderHelper
    {
    public:
        static Common::ErrorCode GetRules(_Out_ INetFwPolicy2** fwPolicy, _Out_ INetFwRules** pFwRules);

        static Common::ErrorCode GetOrRemoveFirewallRules(
            _Inout_ std::vector<BSTR> & rules,
            std::wstring const& groupName = L"",
            uint64 nodePrefix = 0,
            bool isRemove = false);
        static Common::ErrorCode RemoveRules(
            std::vector<BSTR> const & rules,
            Microsoft::WRL::ComPtr<INetFwRules> pFwRules = nullptr);

        //Make sure the thread invoking initialize thread is the one invoking UninitializeThread
        static Common::ErrorCode InitializeCOM();
        static void UninitializeCOM();

        static std::wstring firewallGroup_;

    private:

        static Common::ErrorCode GetOrRemoveFirewallRulesInternal(
            _Out_ std::vector<BSTR> & rules,
            std::wstring const& groupName,
            uint64 nodePrefix,
            bool isRemove);

        static Common::ErrorCode RemoveRulesInternal(
            std::vector<BSTR> const & rules,
            Microsoft::WRL::ComPtr<INetFwRules> pFwRules);
    };
}
