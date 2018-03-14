// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FirewallSecurityProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FirewallSecurityProvider)

    public:
        FirewallSecurityProvider(
            Common::ComponentRoot const & root);
        virtual ~FirewallSecurityProvider();

        Common::ErrorCode ConfigurePortFirewallPolicy(
            std::wstring const & policyName,
            std::vector<LONG> ports);

        Common::ErrorCode RemoveFirewallRule(
            std::wstring const & policyName);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        HRESULT AddRule(
            INetFwRules* rules,
            std::wstring const & ruleName,
            std::wstring const & ruleDescription,
            LONG currentProfileBitMask,
            LONG protocol,
            LONG port,
            bool outgoing);
        
        Common::ErrorCode CleanupRulesForAllProfiles(std::wstring const & policyName, INetFwRules* rules);

        static std::wstring GetFirewallRuleName(
			std::wstring const & policyName,
			bool outgoing,
			LONG profileType,
			LONG protocol);


    private:
        static std::wstring firewallGroup_;
        static LONG allProfiles_[3];
        std::vector<LONG> allprotocols_;
        std::vector<LONG> profilesEnabled_;

    };
}
