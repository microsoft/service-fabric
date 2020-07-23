// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using NetFwTypeLib;
    using System;

    /// <summary>
    /// This clas provides specifications for V1 rules.
    /// </summary>
    class V1FabricFirewallRuleSpecifications
    {
        public static bool RuleIsV1FabricFirewallRule(NetFwRule rule)
        {
            return rule.Name.IndexOf(FabricExceptionSubString, 0, StringComparison.OrdinalIgnoreCase) != -1
                || rule.Name.IndexOf(LeaseDriverExceptionSubString, 0, StringComparison.OrdinalIgnoreCase) != -1
                || rule.Name.IndexOf(ApplicationPortRangeExceptionTCPSubString, 0, StringComparison.OrdinalIgnoreCase) != -1
                || rule.Name.IndexOf(ApplicationPortRangeExceptionUDPSubString, 0, StringComparison.OrdinalIgnoreCase) != -1
                || (!string.IsNullOrEmpty(rule.ApplicationName) && rule.ApplicationName.IndexOf("Fabric.Code.1.0", 0, StringComparison.OrdinalIgnoreCase) != -1);
        }

        private const string FabricExceptionSubString = @"Fabric Process Exception";
        private const string LeaseDriverExceptionSubString = "Lease Driver Port Exception";
        private const string ApplicationPortRangeExceptionTCPSubString = "Application Port Range Exception For TCP";
        private const string ApplicationPortRangeExceptionUDPSubString = "Application Port Range Exception For UDP";
    }
}