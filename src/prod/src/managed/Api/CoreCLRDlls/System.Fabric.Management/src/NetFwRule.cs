// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NetFwTypeLib
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.Text;
    using NetFwTypeLib;

    internal class NetFwRule : INetFwRule
    {
        public NET_FW_ACTION_ Action { get; set; }

        public string ApplicationName { get; set; }

        public NET_FW_RULE_DIRECTION_ Direction { get; set; }

        public bool Enabled { get; set; }

        public string Grouping { get; set; }

        public string LocalPorts { get; set; }

        public string Name { get; set; }

        public int Profiles { get; set; }

        public int Protocol { get; set; }

        public INetFwRule Clone()
        {
            var rule = new NetFwRule
            {
                Action = this.Action,
                ApplicationName = this.ApplicationName,
                Direction = this.Direction,
                Enabled = this.Enabled,
                Grouping = this.Grouping,
                LocalPorts = this.LocalPorts,
                Name = this.Name,
                Profiles = this.Profiles,
                Protocol = this.Protocol,
            };

            return rule;
        }

        private StringBuilder GetCommandOptions(bool isAdd)
        {
            if (String.IsNullOrEmpty(this.Grouping) || String.IsNullOrEmpty(this.Name))
            {
                throw new ArgumentException();
            }

            StringBuilder commandBuilder = new StringBuilder();
            commandBuilder.Append(" -w 30 ");
            if (isAdd)
            {
                commandBuilder.Append(" -I ");
            }
            else
            {
                commandBuilder.Append(" -D ");
            }
            if (NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN == this.Direction)
            {
                commandBuilder.Append(" INPUT ");
            }
            else if(NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_OUT == this.Direction)
            {
                commandBuilder.Append(" OUTPUT ");
            }
            if (isAdd)
            {
                commandBuilder.Append(" 1 ");
            }
            if (Protocol == 1)
            {
                commandBuilder.AppendFormat(" -p  {0} ", "tcp");
            }
            else if (Protocol == 2)
            {
                commandBuilder.AppendFormat(" -p {0} ", "udp");
            }
            if (LocalPorts.Contains("-"))
            {
                commandBuilder.AppendFormat(" -m multiport --dports {0} ", this.LocalPorts.Replace("-",":"));
            }
            else
            {
                commandBuilder.AppendFormat(" --dport {0} ", this.LocalPorts);
            }
            commandBuilder.AppendFormat(" -m comment --comment  '{0}:{1}'", this.Grouping, this.Name);
            commandBuilder.Append(" -j ACCEPT");
            return commandBuilder;
        }

        internal string AddCommand()
        {
            StringBuilder commandBuilderOptions = GetCommandOptions(true);
            StringBuilder command = new StringBuilder();
            command.AppendFormat(" iptables {0} ; ip6tables {0} ", commandBuilderOptions.ToString());
            return command.ToString();
        }

        internal string RemoveCommand()
        {
            StringBuilder commandBuilderOptions = GetCommandOptions(false);
            StringBuilder command = new StringBuilder();
            command.AppendFormat(" iptables {0} ; ip6tables {0} ", commandBuilderOptions.ToString());
            return command.ToString();
        }
    }
}