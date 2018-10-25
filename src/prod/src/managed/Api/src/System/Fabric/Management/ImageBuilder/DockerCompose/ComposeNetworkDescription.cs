// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using YamlDotNet.RepresentationModel;

    //
    // This is given as a root level element in the docker compose file.
    // networks:
    //   <networkname>:
    //     driver: 
    //
    // or
    // networks:
    //   default:
    //     external:
    //       name: <name>
    //
    internal class ComposeNetworkDescription : ComposeObjectDescription
    {
        private const string NameKey = "name";

        public string NetworkName;

        // This is set to specify an existing network name.
        public string ExternalNetworkName;

        public bool IsDefaultNetwork
        {
            get { return this.NetworkName == DockerComposeConstants.DefaultNetwork; }
        }

        public ComposeNetworkDescription(string networkName)
        {
            this.NetworkName = networkName;
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var rootNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "", node);
            foreach (var item in rootNode.Children)
            {
                switch (item.Key.ToString())
                {
                    case DockerComposeConstants.ExternalNetworkDriver:
                    {
                        ParseExternalNetworkNode(
                            DockerComposeUtils.GenerateTraceContext(traceContext,
                                DockerComposeConstants.ExternalNetworkDriver),
                            DockerComposeUtils.ValidateAndGetMapping(
                                traceContext,
                                DockerComposeConstants.ExternalNetworkDriver,
                                item.Value),
                            ignoredKeys);

                        break;
                    }
                    default:
                    {
                        // Parse custom network's when we add that support in our product
                        break;
                    }
                }
            }
        }

        public override void Validate()
        {
            if (!string.IsNullOrEmpty(this.NetworkName) && !this.IsDefaultNetwork)
            {
                throw new FabricComposeException("Only default network option supported in global 'Networks' section");
            }

            if (!string.IsNullOrEmpty(this.ExternalNetworkName) &&
                this.ExternalNetworkName != DockerComposeConstants.OpenExternalNetworkValue &&
                this.ExternalNetworkName != DockerComposeConstants.NatExternalNetworkValue)
            {
                throw new FabricComposeException("Only 'nat' or 'open' are supported as external networks in the 'Networks' section");
            }
        }

        private void ParseExternalNetworkNode(
            string context, 
            YamlMappingNode externalNetworkNode,
            HashSet<string> ignoredKeys)
        {
            foreach (var item in externalNetworkNode.Children)
            {
                switch (item.Key.ToString())
                {
                    case NameKey:
                    {
                        this.ExternalNetworkName = item.Value.ToString();
                        break;
                    }
                    default:
                    {
                        ignoredKeys.Add(item.Key.ToString());
                        break;
                    }
                }
            }
        }
    }
}