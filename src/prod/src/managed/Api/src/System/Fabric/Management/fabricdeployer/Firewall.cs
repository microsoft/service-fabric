// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using NetFwTypeLib;

    /// <summary>
    /// This is the class that handles the creation of server deployment on a single node based on the
    /// input manifest.
    /// </summary>
    internal class Firewall
    {
#if !DotNetCoreClrLinux
        private const string FirewallService = "MpsSvc";
        private const string TypeProgID = "HNetCfg.FwPolicy2";
        private static Type policyType = Type.GetTypeFromProgID(TypeProgID);
        private INetFwPolicy2 policy;
#endif
        private INetFwRules rules;

        public Firewall()
        {
#if !DotNetCoreClrLinux
            policy = (INetFwPolicy2)Activator.CreateInstance(policyType);
            rules = policy.Rules;
#else
            rules = NetFwRules.GetAllRules();
#endif
        }

        public bool IsEnabled()
        {

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            System.ServiceProcess.ServiceController controller = new System.ServiceProcess.ServiceController(FirewallService);
            return controller.Status == System.ServiceProcess.ServiceControllerStatus.Running
                || policy.get_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_DOMAIN)
                || policy.get_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PRIVATE)
                || policy.get_FirewallEnabled(NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_PUBLIC);
#else
            // For now assume firewall is always enabled on Linux
            return true;
#endif
        }

        public void UpdateRules(List<FirewallRule> newRules, bool removeRulesIfNotRequired)
        {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            RemoveV1Rules();
#endif
            if (removeRulesIfNotRequired)
            {
                RemoveRulesNotRequried(newRules);
            }

            foreach (var newRule in newRules)
            {
                AddOrUpdateRule(newRule);
            }
        }

        public void RemoveWindowsFabricRules()
        {
#if !DotNetCoreClrIOT
            List<string> windowsFabricRuleNames = new List<string>();
            foreach (var rule in this.rules)
            {
                NetFwRule fwRule = (NetFwRule)rule;
                if (FabricNodeFirewallRules.IsFabricFirewallRule(fwRule))
                {
                    windowsFabricRuleNames.Add(fwRule.Name);
                }
            }

            foreach (var ruleName in windowsFabricRuleNames)
            {
                this.rules.Remove(ruleName);
            }
#endif
        }

        private void RemoveRulesNotRequried(List<FirewallRule> newRules)
        {
#if !DotNetCoreClrIOT
            List<string> rulesToBeDeleted = new List<string>();
            foreach (var rule in this.rules)
            {
                NetFwRule fwRule = (NetFwRule)rule;
                if (fwRule == null)
                {
                    continue;
                }
                if (FabricNodeFirewallRules.IsFabricFirewallRule(fwRule))
                {
                    if (newRules.All(newRule => newRule.Name != fwRule.Name)) // Firewall rule is not in the set of new rules
                    {
                        rulesToBeDeleted.Add(fwRule.Name);
                    }
                }
            }
            foreach (string ruleToBeDeleted in rulesToBeDeleted)
            {
                rules.Remove(ruleToBeDeleted);
            }
#endif 
        }

        private void AddOrUpdateRule(FirewallRule newRule)
        {
#if !DotNetCoreClrIOT
            try
            {
                bool addRule = true;
#if DotNetCoreClrLinux
                bool updateRuleForLinux = false;
#endif
                INetFwRule fwRule = null;
                foreach (var rule in rules)
                {
                    fwRule = (INetFwRule)rule;
                    if (fwRule.Name == newRule.Name)
                    {
                        addRule = false;
                        if (String.IsNullOrEmpty(newRule.ApplicationPath)
                            || String.IsNullOrEmpty(fwRule.ApplicationName)
                            || fwRule.ApplicationName != newRule.ApplicationPath)
                        {
                            fwRule.ApplicationName = newRule.ApplicationPath;
                        }

                        if (String.IsNullOrEmpty(newRule.Ports)
                            || String.IsNullOrEmpty(fwRule.LocalPorts)
                            || fwRule.LocalPorts != newRule.Ports)
                        {
                            fwRule.LocalPorts = newRule.Ports;
#if DotNetCoreClrLinux
                            updateRuleForLinux = true;
#endif
                        }

                        break;
                    }
                }

                if (addRule)
                {
                    NetFwRule netFwRule = newRule.GetNetFwRule() as NetFwRule;
                    rules.Add(netFwRule);
                }
#if DotNetCoreClrLinux
                else if (null != fwRule && updateRuleForLinux)
                {
                    rules.Update(fwRule);
                }
#endif
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Error encountered in AddorUpdateRule: {0}", ex.ToString());
                throw;
            }
#endif
        }

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
        private void RemoveV1Rules()
        {
            List<string> V1RulesToBeRemoved = new List<string>();
            foreach (var rule in rules)
            {
                NetFwRule fwRule = (NetFwRule)rule;
                if (V1FabricFirewallRuleSpecifications.RuleIsV1FabricFirewallRule(fwRule))
                {
                    V1RulesToBeRemoved.Add(fwRule.Name);
                }
            }
            foreach (string ruleName in V1RulesToBeRemoved)
            {
                rules.Remove(ruleName);
            }
        }
#endif
    }
}