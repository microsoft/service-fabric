// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.IO;
    using YamlDotNet.RepresentationModel;

    /// <summary>
    /// Representation for a Container Service.
    /// </summary>
    internal class ComposeServiceTypeDescription : ComposeObjectDescription
    {
        public string TypeName;

        public string TypeVersion;

        public string ImageName;

        public string EntryPointOverride;

        public string Commands;

        public IDictionary<string, string> EnvironmentVariables;

        public IList<ComposeServicePorts> Ports;

        public IList<string> PlacementConstraints;

        public ComposeServiceResourceGovernance ResourceGovernance;

        public ComposeServiceLoggingOptions LoggingOptions;

        public int InstanceCount;

        public IList<ComposeServiceVolume> VolumeMappings;

        public string Isolation;

        public ComposeServiceTypeDescription(string name, string version)
        {
            this.Ports = new List<ComposeServicePorts>();
            this.EnvironmentVariables = new Dictionary<string, string>();
            this.PlacementConstraints = new List<string>();
            this.ImageName = null;
            this.Isolation = null;
            this.TypeName = name;
            this.TypeVersion = version;
            this.InstanceCount = 1;
            this.VolumeMappings = new List<ComposeServiceVolume>();
            this.ResourceGovernance = new ComposeServiceResourceGovernance();
            this.LoggingOptions = new ComposeServiceLoggingOptions();
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var serviceRootNode = DockerComposeUtils.ValidateAndGetMapping(traceContext, "", node);

            foreach (var childItem in serviceRootNode.Children)
            {
                var key = childItem.Key.ToString();
                switch (childItem.Key.ToString())
                {
                    case DockerComposeConstants.ImageKey:
                    {
                        this.ImageName = DockerComposeUtils.ValidateAndGetScalar(
                            traceContext,
                            DockerComposeConstants.ImageKey,
                            childItem.Value).ToString();

                        this.ParseImageTag(this.ImageName);
                        break;
                    }
                    case DockerComposeConstants.PortsKey:
                    {
                        this.ParsePorts(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.PortsKey),
                            DockerComposeUtils.ValidateAndGetSequence(traceContext, DockerComposeConstants.PortsKey, childItem.Value),
                            ignoredKeys);

                        break;
                    }
                    case DockerComposeConstants.DeployKey:
                    {
                        this.ParseDeploymentParameters(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.DeployKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.DeployKey, childItem.Value),
                            ignoredKeys);

                        break;
                    }
                    case DockerComposeConstants.EntryPointKey:
                    {
                        this.EntryPointOverride = DockerComposeUtils.ValidateAndGetScalar(
                            traceContext,
                            DockerComposeConstants.ImageKey,
                            childItem.Value).ToString(); 

                        break;
                    }
                    case DockerComposeConstants.CommandKey:
                    {
                        this.Commands = DockerComposeUtils.ValidateAndGetScalar(
                            traceContext,
                            DockerComposeConstants.ImageKey,
                            childItem.Value).ToString();

                        break;
                    }
                    case DockerComposeConstants.LoggingKey:
                    {
                        DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.LoggingKey, childItem.Value);

                        this.LoggingOptions.Parse(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.LoggingKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.LoggingKey, childItem.Value),
                            ignoredKeys);

                        break;
                    }
                    case DockerComposeConstants.VolumesKey:
                    {
                        this.ParseVolumeMappings(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.VolumesKey),
                            DockerComposeUtils.ValidateAndGetSequence(traceContext, DockerComposeConstants.VolumesKey, childItem.Value),
                            ignoredKeys);

                        break;
                    }
                    case DockerComposeConstants.EnvironmentKey:
                    {
                        if (childItem.Value.NodeType == YamlNodeType.Sequence)
                        {
                            this.ParseEnvironment(
                                DockerComposeUtils.GenerateTraceContext(traceContext,
                                    DockerComposeConstants.EnvironmentKey),
                                DockerComposeUtils.ValidateAndGetSequence(traceContext,
                                    DockerComposeConstants.EnvironmentKey, childItem.Value));
                        }
                        else if (childItem.Value.NodeType == YamlNodeType.Mapping)
                        {
                                this.ParseEnvironment(
                                    DockerComposeUtils.GenerateTraceContext(traceContext,
                                        DockerComposeConstants.EnvironmentKey),
                                    DockerComposeUtils.ValidateAndGetMapping(traceContext,
                                        DockerComposeConstants.EnvironmentKey, childItem.Value));
                        }
                        else
                        {
                                throw new FabricComposeException(
                                    string.Format("{0} - {1} expects a sequence or mapping element.", traceContext, DockerComposeConstants.EnvironmentKey));
                        }
                        break;
                    }
                    case DockerComposeConstants.IsolationKey:
                    {
                        this.Isolation = DockerComposeUtils.ValidateAndGetScalar(
                            traceContext,
                            DockerComposeConstants.IsolationKey,
                            childItem.Value).ToString();

                        if (this.Isolation != DockerComposeConstants.IsolationValueDefault &&
                            this.Isolation != DockerComposeConstants.IsolationValueProcess &&
                            this.Isolation != DockerComposeConstants.IsolationValueHyperv)
                        {
                            throw new FabricComposeException(string.Format("{0} - Invalid value {1} specified for isolation", traceContext, this.Isolation));
                        }
                        break;
                    }
                    case DockerComposeConstants.LabelsKey:
                    {
                        // ignored.
                        break;
                    }
                    default:
                    {
                        // TODO: logging.
                        ignoredKeys.Add(key);
                        break;
                    }
                }
            }
        }

        public override void Validate()
        {
            //
            // We need atleast an image key to construct the code package.
            //
            if (string.IsNullOrEmpty(this.ImageName))
            {
                throw new FabricComposeException(string.Format("'image' key not specified or empty for service {0}", this.TypeName));
            }

            foreach (var port in this.Ports)
            {
                port.Validate();
            }

            foreach (var volume in this.VolumeMappings)
            {
                volume.Validate();
            }

            this.ResourceGovernance.Validate();
            this.LoggingOptions.Validate();
        }

        private void ParseImageTag(string imageName)
        {
            var index = imageName.LastIndexOf(DockerComposeConstants.SemiColonDelimiter, imageName.Length - 1,
                StringComparison.Ordinal);
            if (index == -1)
            {
                return;
            }
        }

        private void ParsePorts(string traceContext, YamlSequenceNode portMappings, HashSet<string> ignoredKeys)
        {
            foreach (var item in portMappings)
            {
                var portMapping = new ComposeServicePorts();
                portMapping.Parse(traceContext, item, ignoredKeys);
                this.Ports.Add(portMapping);
            }
        }

        private void ParseEnvironment(string traceContext, YamlSequenceNode environmentVariables)
        {
            foreach (var environmentVariable in environmentVariables)
            {
                var keyValue = environmentVariable.ToString().Split(DockerComposeConstants.EqualsDelimiter, 2, StringSplitOptions.RemoveEmptyEntries);
                if (keyValue.Length != 2)
                {
                    throw new FabricComposeException(
                        string.Format("{0} - Unable to parse environment variable '{1}'",
                        traceContext,
                        environmentVariable.ToString()));
                }

                this.EnvironmentVariables.Add(keyValue[0], keyValue[1]);
            }
        }

        private void ParseEnvironment(string traceContext, YamlMappingNode environmentVariables)
        {
            foreach (var environment in environmentVariables)
            {
                this.EnvironmentVariables.Add(environment.Key.ToString(), environment.Value.ToString());
            }
        }

        private void ParseVolumeMappings(string traceContext, YamlSequenceNode volumeMappings, HashSet<string> ignoredKeys)
        {
            foreach (var item in volumeMappings)
            {
                var volumeMapping = new ComposeServiceVolume();
                volumeMapping.Parse(traceContext, item, ignoredKeys);
                this.VolumeMappings.Add(volumeMapping);
            }
        }

        internal void ParseDeploymentParameters(string traceContext, YamlMappingNode rootDeployNode, HashSet<string> ignoredKeys)
        {
            foreach (var item in rootDeployNode)
            {
                var key = item.Key.ToString();
                switch (key)
                {
                    case DockerComposeConstants.DeploymentModeKey:
                    {
                        var value = item.Value.ToString();
                        if (value != DockerComposeConstants.DeploymentModeReplicatedValue)
                        {
                            throw new FabricComposeException(
                                string.Format("{0} - Only 'replicated' deployment mode is supported. Specified {1}", traceContext, value));
                        }
                        break;
                    }
                    case DockerComposeConstants.ReplicasKey:
                    {
                        var value = DockerComposeUtils.ValidateAndGetScalar(traceContext, DockerComposeConstants.ReplicasKey, item.Value).ToString();
                        try
                        {
                            this.InstanceCount = Int32.Parse(value);
                        }
                        catch (Exception e)
                        {
                            throw new FabricComposeException(string.Format("{0} - Parsing 'replicas' with value {1} failed", e, value));
                        }
                        break;
                    }
                    case DockerComposeConstants.PlacementKey:
                    {
                        this.ParsePlacementConstraints(
                            DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.PlacementKey),
                            DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.PlacementKey, item.Value));
                        break;
                    }
                    case DockerComposeConstants.ResourcesKey:
                        {
                            this.ParseResourceGovernance(
                                DockerComposeUtils.GenerateTraceContext(traceContext, DockerComposeConstants.ResourcesKey),
                                DockerComposeUtils.ValidateAndGetMapping(traceContext, DockerComposeConstants.ResourcesKey, item.Value),
                                ignoredKeys);
                            break;
                        }
                    case DockerComposeConstants.LabelsKey:
                    {
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

        private void ParseResourceGovernance(string traceContext, YamlMappingNode resources, HashSet<string> ignoredKeys)
        {
            this.ResourceGovernance.Parse(traceContext, resources, ignoredKeys);
        }

        private void ParsePlacementConstraints(string traceContext, YamlMappingNode placementConstraints)
        {
            foreach (var placementConstraintItem in placementConstraints)
            {
                var constraints = DockerComposeUtils.ValidateAndGetSequence(traceContext, "", placementConstraintItem.Value);
                foreach (var constraint in constraints)
                {
                    this.PlacementConstraints.Add(constraint.ToString());
                }
            }
        }
    }
}
