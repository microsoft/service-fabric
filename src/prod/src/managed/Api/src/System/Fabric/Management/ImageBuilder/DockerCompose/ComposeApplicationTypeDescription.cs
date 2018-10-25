// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.IO;
    using YamlDotNet.RepresentationModel;

    internal class ComposeApplicationTypeDescription : ComposeObjectDescription
    {
        public string DockerComposeVersion;

        public string ApplicationTypeName;

        public string ApplicationTypeVersion;

        public IDictionary<string, ComposeServiceTypeDescription> ServiceTypeDescriptions;

        public IDictionary<string, ComposeVolumeDescription> VolumeDescriptions;

        public IDictionary<string, ComposeNetworkDescription> NetworkDescriptions;

        public ComposeApplicationTypeDescription()
        {
            this.ServiceTypeDescriptions = new Dictionary<string, ComposeServiceTypeDescription>();
            this.VolumeDescriptions = new Dictionary<string, ComposeVolumeDescription>();
            this.ApplicationTypeVersion = DateTime.Now.Ticks.ToString();
            this.ApplicationTypeName = DockerComposeConstants.DefaultApplicationTypeName;
            this.NetworkDescriptions = new Dictionary<string, ComposeNetworkDescription>();
            this.DockerComposeVersion = null;
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var rootNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "Root", node);

            foreach (var childItem in rootNode.Children)
            {
                var key = childItem.Key.ToString();
                switch (key)
                {
                    case DockerComposeConstants.VersionKey:
                    {
                        this.DockerComposeVersion = childItem.Value.ToString();

                        if (!this.DockerComposeVersion
                                .StartsWith(DockerComposeConstants.SupportedDockerComposeVersion3) &&
                            !this.DockerComposeVersion
                                .StartsWith(DockerComposeConstants.SupportedDockerComposeVersion2))
                        {
                            throw new FabricComposeException(
                                string.Format(
                                    "Docker compose file version not supported. Supported version - '3' input file version - '{0}'",
                                    childItem.Value.ToString()));
                        }

                        break;
                    }
                    case DockerComposeConstants.ServicesKey:
                    {
                        this.ParseServices(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.ServicesKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.ServicesKey, childItem.Value),
                            ignoredKeys);
                        break;
                    }
                    case DockerComposeConstants.VolumesKey:
                    {
                        this.ParseVolumes(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.VolumesKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.VolumesKey, childItem.Value),
                            ignoredKeys);
                        break;
                    }
                    case DockerComposeConstants.LabelsKey:
                    {
                        this.ParseLabels(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.LabelsKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.LabelsKey, childItem.Value));
                        break;
                    }
                    case DockerComposeConstants.NetworksKey:
                    {
                        this.ParseNetworks(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.NetworksKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.NetworksKey, childItem.Value),
                            ignoredKeys);
                        break;
                    }
                    default:
                    {
                        ignoredKeys.Add(key);
                        break;
                    }
                }
            }
        }

        public override void Validate()
        {
            // Version must be specified.
            if (string.IsNullOrEmpty(this.DockerComposeVersion))
            {
                throw new FabricComposeException(string.Format("'version' key not specified."));
            }

            // Service can't be empty
            if (this.ServiceTypeDescriptions.Count == 0)
            {
                throw new FabricComposeException("'services' must be specified and not empty.");
            }

            foreach (var serviceDescription in this.ServiceTypeDescriptions)
            {
                serviceDescription.Value.Validate();
            }

            foreach (var volumeDescription in this.VolumeDescriptions)
            {
                volumeDescription.Value.Validate();
            }

            foreach (var networkDescription in this.NetworkDescriptions)
            {
                networkDescription.Value.Validate();
                if (networkDescription.Value.IsDefaultNetwork &&
                    networkDescription.Value.ExternalNetworkName == DockerComposeConstants.OpenExternalNetworkValue)
                {
                    foreach (var serviceDescription in this.ServiceTypeDescriptions)
                    {
                        if (serviceDescription.Value.Ports.Count != 0)
                        {
                            throw new FabricComposeException(
                                string.Format("Service '{0}' : 'Ports' and default external network name 'open' are not compatible.", serviceDescription.Key.ToString()));
                        }
                    }
                }
            }
        }

        private void ParseServices(string context, YamlMappingNode servicesRootNode, HashSet<string> ignoredKeys)
        {
            foreach (var childItem in servicesRootNode.Children)
            {
                var name = childItem.Key.ToString();
                DockerComposeUtils.ValidateAndGetMapping(context, name, childItem.Value);

                var serviceTypeDesc = new ComposeServiceTypeDescription(name, this.ApplicationTypeVersion);
                serviceTypeDesc.Parse(
                    DockerComposeUtils.GenerateTraceContext(context, name),
                    childItem.Value,
                    ignoredKeys);

                this.ServiceTypeDescriptions.Add(name, serviceTypeDesc);
            }
        }

        private void ParseVolumes(string context, YamlMappingNode volumes, HashSet<string> ignoredKeys)
        {
            foreach (var volume in volumes)
            {
                DockerComposeUtils.ValidateAndGetMapping(context, volume.Key.ToString(), volume.Value);

                var volumeDescription = new ComposeVolumeDescription();
                volumeDescription.Parse(context, volume.Value, ignoredKeys);
                this.VolumeDescriptions.Add(volume.Key.ToString(), volumeDescription);
            }
        }

        private void ParseNetworks(string context, YamlMappingNode networks, HashSet<string> ignoredKeys)
        {
            foreach (var network in networks)
            {
                DockerComposeUtils.ValidateAndGetMapping(context, network.Key.ToString(), network.Value);

                var networkDescription = new ComposeNetworkDescription(network.Key.ToString());
                networkDescription.Parse(context, network.Value, ignoredKeys);
                this.NetworkDescriptions.Add(network.Key.ToString(), networkDescription);
            }
        }

        private void ParseLabels(string context, YamlMappingNode node)
        {
            foreach (var label in node.Children)
            {
                switch (label.Key.ToString())
                {
                    case DockerComposeConstants.SfApplicationTypeNameKey:
                    {
                        this.ApplicationTypeName = label.Value.ToString();
                        break;
                    }
                    case DockerComposeConstants.SfApplicationTypeVersionKey:
                    {
                        this.ApplicationTypeVersion = label.Value.ToString();
                        break;
                    }
                }
            }
        }
    }
}