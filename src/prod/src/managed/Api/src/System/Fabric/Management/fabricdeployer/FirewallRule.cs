// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using NetFwTypeLib;
    using System;

    /// <summary>
    /// This is the class wrapping up the NetFwTypeLib firewall rule class.
    /// </summary>
    class FirewallRule
    {
        private const string TypeProgID = "HNetCfg.FWRule";
        public string Name { get; set; }
        public string Ports { get; set; }
        public NET_FW_IP_PROTOCOL_ Protocol { get; set; }
        public NET_FW_RULE_DIRECTION_ Direction { get; set; }
        public NET_FW_PROFILE_TYPE2_ Profile { get; set; }
        public string ApplicationPath { get; set; }
        public string Grouping { get; set; }

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
        static Type ruleType = Type.GetTypeFromProgID(FirewallRule.TypeProgID);
#else
        static Type ruleType = typeof(NetFwRule);
#endif

        public INetFwRule GetNetFwRule()
        {
            INetFwRule rule = (INetFwRule)Activator.CreateInstance(ruleType);
            rule.Name = this.Name;
            rule.Protocol = (int)this.Protocol;
            rule.Direction = this.Direction;
            rule.ApplicationName = this.ApplicationPath;
            rule.LocalPorts = this.Ports;
            rule.Grouping = this.Grouping;
            rule.Enabled = true;
            rule.Profiles = (int)Profile;

            return rule;
        }
    }
}