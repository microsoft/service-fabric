// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NetFwTypeLib
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric.Management.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Runtime.InteropServices;
    using System.Text;


    internal class NetFwRules : INetFwRules
    {
        private IDictionary<string, INetFwRule> _rules;

        private NetFwRules()
        {
            _rules = new Dictionary<string, INetFwRule>();
            PopulateFwRules();
        }

        private void PopulateFwRules()
        {
#if DotNetCoreClrLinux
            string listCommand = "iptables -w -L --line-numbers";
            IntPtr pipePtr = NativeHelper.popen(listCommand, "r");
            if (pipePtr == IntPtr.Zero)
            {
                throw new Exception(String.Format("Error encountered in opening a pipe to execute command {0}", listCommand));
            }
            try
            {
                StringBuilder outputBuilder = new StringBuilder(1024);
                int type = 0;
                while (NativeHelper.fgets(outputBuilder, 1024, pipePtr) != IntPtr.Zero)
                {
                    string sData = outputBuilder.ToString();
                    if (sData.Contains("Chain INPUT"))
                    {
                        type = 1;
                    }
                    else if (sData.Contains("Chain OUTPUT"))
                    {
                        type = 2;
                    }
                    if (sData.Contains("WindowsFabric:"))
                    {
                        NetFwRule netFwRule = new NetFwRule();
                        int commentStart = sData.IndexOf("/*");
                        int commentend = sData.IndexOf("*/");
                        string comment = sData.Substring(commentStart + 2, commentend - (commentStart + 2)).Trim();
                        string[] commentArray = comment.Split(':');
                        netFwRule.Grouping = commentArray[0];
                        netFwRule.Name = commentArray[1];

                        if (this._rules.ContainsKey(netFwRule.Name))
                        {
                            // Duplicate rule detected
                            continue;
                        }

                        if (sData.Contains("tcp"))
                        {
                            netFwRule.Protocol = 1;
                        }
                        else
                        {
                            netFwRule.Protocol = 2;
                        }
                        int dptStart = sData.IndexOf("dpt:");
                        if (dptStart != -1)
                        {
                            string port = sData.Substring(dptStart + 4, commentStart - (dptStart + 4)).Trim();
                            netFwRule.LocalPorts = LookupPort(port);
                        }
                        else
                        {
                            int multiportStart = sData.IndexOf("dports");
                            if (multiportStart == -1)
                            {
                                throw new Exception(String.Format("Firewall line contained neither dpt nor dports: {0}", sData));
                            }

                            string ports = sData.Substring(multiportStart + 6, commentStart - (multiportStart + 6)).Trim();

                            string[] portsArray = ports.Split(':');
                            portsArray[0] = LookupPort(portsArray[0]);
                            portsArray[1] = LookupPort(portsArray[1]);

                            netFwRule.LocalPorts = string.Format("{0}-{1}", portsArray[0], portsArray[1]);
                        }
                        if (type == 1)
                        {
                            netFwRule.Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN;
                        }
                        else
                        {
                            netFwRule.Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT;
                        }

                        netFwRule.Action = NET_FW_ACTION_.NET_FW_ACTION_ALLOW;      // Assuming today we create all allow rules
                        this._rules.Add(netFwRule.Name, netFwRule);
                    }
                }
            }
            finally
            {
                NativeHelper.pclose(pipePtr);
            }
#endif
        }

        public int Count { get { return _rules.Count; } }

        public void Add(INetFwRule rule)
        {
            if (String.IsNullOrEmpty(rule.LocalPorts) || rule.LocalPorts.Equals("0")) return;

            NetFwRule netFwRule = (NetFwRule) rule;
#if DotNetCoreClrLinux
            var addCommand = netFwRule.AddCommand();
            int returnvalue  = NativeHelper.system(addCommand);
            if (returnvalue != 0)
            {
                throw new Exception(String.Format("Error encountered in executing command {0} with return value {1}", addCommand, returnvalue));
            }
#endif
            _rules.Add(rule.Name, rule.Clone());
        }

        public IEnumerator GetEnumerator()
        {
            foreach (var rule in _rules.Values)
            {
                yield return rule.Clone();
            }
        }

        public INetFwRule Item(string name)
        {
            var rule = _rules[name];
            return (rule == null) ? null : rule.Clone();
        }

        public void Remove(string name)
        {
            NetFwRule netFwRule = (NetFwRule)this._rules[name];
#if DotNetCoreClrLinux
            NativeHelper.system(netFwRule.RemoveCommand());     // Ignore any error encountered in removing a rule
#endif
            _rules.Remove(name);
        }

        public void Update(INetFwRule rule)
        {
            this.Remove(rule.Name);
            this.Add(rule);
        }

        internal static NetFwRules GetAllRules()
        {
            var fwRules = new NetFwRules();
            return fwRules;
        }

        private string LookupPort(string port)
        {
            if (!Int32.TryParse(port, out int value))
            {
                // Scan /etc/services file for service name
                return LookupPortForServiceName(port);
            }

            return port;
        }

        private string LookupPortForServiceName(string netServiceName)
        {
            string command = string.Format(@"getent services {0} | sed -n 's/.*\s\([0-9]\+\).*/\1/p'", netServiceName);
            IntPtr pipePtr = NativeHelper.popen(command, "r");
            if (pipePtr == IntPtr.Zero)
            {
                throw new Exception(String.Format("Error encountered in opening a pipe to execute command {0}", command));
            }

            try
            {
                StringBuilder outputBuilder = new StringBuilder(1024);
                if (NativeHelper.fgets(outputBuilder, 1024, pipePtr) != IntPtr.Zero)
                {
                    return outputBuilder.ToString().Trim();
                }
                else
                {
                    throw new Exception(String.Format("Service port query {0} returned IntPtr.Zero", command));
                }
            }
            finally
            {
                if (pipePtr != IntPtr.Zero)
                {
                    NativeHelper.pclose(pipePtr);
                }
            }
        }
    }
}