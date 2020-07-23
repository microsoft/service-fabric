// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.VMMWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.FabricDeployer;
    using System.Globalization;
    using System.Security.Principal;
    using Microsoft.Win32;
    using System.Net;
    using System.Fabric.Management.ServiceModel;

    internal class VMMNodeList
    {
        public static List<InfrastructureNodeType> Create(string ServiceVMComputerName)
        {
            ServiceVMComputerName = ServiceVMComputerName.Trim();
            if (ServiceVMComputerName.Length == 0)
            {
                return null;
            }

            var nodeList = new List<InfrastructureNodeType>();
            string[] tiers = ServiceVMComputerName.Split("]".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
            System.Console.WriteLine("tiers are: {0}", tiers);

            foreach (string tier in tiers)
            {
                if (tier.Trim().Length == 0)
                {
                    continue;
                }
                string[] tierInfo = tier.Split('[');
                string tierName = tierInfo[0];
                string[] nodes = tierInfo[1].Split(',');
                
                foreach (string node in nodes)
                {
                    string[] nodeinfo = node.Split(':');
                    string nodeName = nodeinfo[0];
                    string upgradDomain = nodeinfo[1];
                    InfrastructureNodeType n = new InfrastructureNodeType
                    {
                        FaultDomain = GetFaultDomain(nodeName),
                        UpgradeDomain = upgradDomain,
                        IPAddressOrFQDN = nodeName,
                        NodeName = nodeName,
                        RoleOrTierName = tierName,
                        IsSeedNode = true,
                        NodeTypeRef = string.Empty
                    };
                    nodeList.Add(n);
                }
            }

            return nodeList;
        }

        public static string GetFQDN()
        {
            string strFQDN = "";
            if (strFQDN.Length > 0)
            {
                return strFQDN;
            }
            else
            {
                string domainName = System.Net.NetworkInformation.IPGlobalProperties.GetIPGlobalProperties().DomainName;
                string hostName = Dns.GetHostName();
                if (!hostName.Contains(domainName))
                {
                    return hostName + "." + domainName;
                }
                else
                {
                    return hostName;
                }
            }
        }

        private static string GetFaultDomain(string machineName)
        {
            string strFQDN = GetFQDN();
            System.Console.WriteLine("node mNachineName {0}, FQDN is {1}", machineName, strFQDN);

            if (string.Compare(machineName, strFQDN, true, CultureInfo.InvariantCulture) == 0)
            {
                RegistryKey masterKey = Registry.LocalMachine.CreateSubKey(@"SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters");
                if (masterKey == null)
                {
                    return "";
                }
                else
                {
                    return "fd:/" + masterKey.GetValue("HostName").ToString();
                }
            }

            return "";
        }

    }
}