// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Newtonsoft.Json;

    internal abstract class StandAloneInstallerJsonModelBase
    {
        private static Tuple<string, Type>[] apiVersionTable = new Tuple<string, Type>[]
        {
            new Tuple<string, Type>(StandAloneInstallerJsonModelGA.ModelApiVersion, typeof(StandAloneInstallerJsonModelGA)),
            new Tuple<string, Type>(StandAloneInstallerJSONModelJan2017.ModelApiVersion, typeof(StandAloneInstallerJSONModelJan2017)),
            new Tuple<string, Type>(StandAloneInstallerJSONModelApril2017.April2017ModelApiVersion, typeof(StandAloneInstallerJSONModelApril2017)),
            new Tuple<string, Type>(StandAloneInstallerJSONModelMay2017.ModelApiVersion, typeof(StandAloneInstallerJSONModelMay2017)),
            new Tuple<string, Type>(StandAloneInstallerJSONModelAugust2017.ModelApiVersion, typeof(StandAloneInstallerJSONModelAugust2017)),
            new Tuple<string, Type>(StandAloneInstallerJSONModelOctober2017.ModelApiVersion, typeof(StandAloneInstallerJSONModelOctober2017)),

            // note: must put the latest version as the last element
            new Tuple<string, Type>(StandAloneInstallerJSONModelJune2018.ModelApiVersion, typeof(StandAloneInstallerJSONModelJune2018)),
        };

        public StandAloneInstallerJsonModelBase()
        {
        }

        public StandAloneInstallerJsonModelBase(IUserConfig config, IClusterTopology topology, string configurationVersion)
        {
            this.ClusterConfigurationVersion = configurationVersion;
            this.Name = config.ClusterName;
            this.Nodes = new List<NodeDescription>(topology.Nodes.Values).ConvertAll<NodeDescriptionGA>(node => NodeDescriptionGA.ReadFromInternal(node));
        }

        public List<NodeDescriptionGA> Nodes
        {
            get;
            set;
        }

        public string ApiVersion
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public string ClusterConfigurationVersion
        {
            get;
            set;
        }

        internal static StandAloneInstallerJsonModelBase GetJsonConfigFromFile(string jsonConfigPath)
        {
            if (string.IsNullOrWhiteSpace(jsonConfigPath) || !System.IO.File.Exists(jsonConfigPath))
            {
                SFDeployerTrace.WriteError(StringResources.Error_BPAJsonPathInvalid, jsonConfigPath);
                return null;
            }

            try
            {
                string json = File.ReadAllText(jsonConfigPath);
                return GetJsonConfigFromString(json);
            }
            catch (Exception e)
            {
                var message = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}", 
                    StringResources.Error_SFJsonConfigInvalid,
                    e.ToString());
                SFDeployerTrace.WriteError(message);
                return null;
            }
        }

        internal static StandAloneInstallerJsonModelBase ConstructByApiVersion(
            IUserConfig config,
            IClusterTopology topology,
            string configurationVersion,
            string apiVersion)
        {
            Type modelType;

            if (string.IsNullOrWhiteSpace(apiVersion))
            {
                // backward compatibility:
                //      At powershell layer, apiVersion is not required.
                //      At REST layer, apiVersion is not required for api-version prior to 6.0.
                modelType = StandAloneInstallerJsonModelBase.apiVersionTable.Last().Item2;
            }
            else
            {
                var entry = StandAloneInstallerJsonModelBase.apiVersionTable.FirstOrDefault(p => p.Item1 == apiVersion);
                if (entry != null)
                {
                    modelType = entry.Item2;
                }
                else
                {
                    throw new NotSupportedException(apiVersion + " is not supported!");
                }
            }

            return (StandAloneInstallerJsonModelBase)Activator.CreateInstance(modelType, config, topology, configurationVersion);
        }

        internal static StandAloneInstallerJsonModelBase GetJsonConfigFromString(string jsonString)
        {
            try
            {
                SFDeployerTrace.WriteNoise(StringResources.Info_BPAConvertingJsonToModel);
                StandAloneInstallerJsonModelBase result = DeserializeJsonConfig(typeof(StandAloneInstallerJsonModelGA), jsonString);

                var entry = StandAloneInstallerJsonModelBase.apiVersionTable.FirstOrDefault(p => p.Item1 == result.ApiVersion);
                if (entry != null)
                {
                    Type modelType = entry.Item2;
                    result = DeserializeJsonConfig(modelType, jsonString);

                    if (DevJsonModel.IsDevCluster(result))
                    {
                        result = DeserializeJsonConfig(typeof(DevJsonModel), jsonString);
                    }

                    return result;
                }
                else
                {
                    SFDeployerTrace.WriteWarning(string.Format("Json parsing: Unrecognized api version '{0}'", result.ApiVersion));
                    throw new NotSupportedException(result.ApiVersion + " is not supported!");
                }
            }
            catch (Exception e)
            {
                var message = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}",
                    StringResources.Error_SFJsonConfigInvalid,
                    e.ToString());
                SFDeployerTrace.WriteError(message);
                return null;
            }
        }

        internal abstract Dictionary<string, string> GetFabricSystemSettings();

        internal abstract string GetClusterRegistrationId();

        internal StandAloneUserConfig GetUserConfig()
        {
            return this.OnGetUserConfig();
        }

        internal void UpdateUserConfig(StandAloneUserConfig userConfig)
        {
            userConfig.TotalNodeCount = this.Nodes.Count;
            userConfig.ClusterName = this.Name;
            userConfig.Version = new UserConfigVersion(this.ClusterConfigurationVersion);
            userConfig.IsScaleMin = this.Nodes.Count != this.Nodes.GroupBy(node => node.IPAddress).Count();
        }

        internal StandAloneClusterTopology GetClusterTopology()
        {
            var clusterTopology = new StandAloneClusterTopology()
            {
                Nodes = new Dictionary<string, NodeDescription>(),
                Machines = new List<string>()
            };

            foreach (var node in this.Nodes)
            {
                if (!clusterTopology.Nodes.Any(n => n.Key == node.NodeName))
                {
                    clusterTopology.Nodes.Add(node.NodeName, node.ConvertToInternal());
                }
                else
                {
                    throw new ValidationException(ClusterManagementErrorCode.NodeNameDuplicateDetected, StringResources.NodeNameDuplicateDetected);
                }
            }

            clusterTopology.Machines = clusterTopology.Nodes.Values.Select<NodeDescription, string>(nd => nd.IPAddress).Distinct().ToList();
            return clusterTopology;
        }

        internal bool IsTestCluster()
        {
            Dictionary<string, string> fabricsettings = this.GetFabricSystemSettings();
            if (fabricsettings.ContainsKey(StringConstants.ParameterName.IsTestCluster) &&
                string.Equals("true", fabricsettings[StringConstants.ParameterName.IsTestCluster], StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return false;
        }

        // Ensure all overriden implementations of FromInternal cover all newly added properties.
        internal virtual void FromInternal(
            IClusterTopology clusterTopology,
            IUserConfig userConfig,
            string apiVersion)
        {
            this.Nodes = new List<NodeDescriptionGA>(
                clusterTopology.Nodes.Values.Select<NodeDescription, NodeDescriptionGA>(
                    node => NodeDescriptionGA.ReadFromInternal(node)));
            this.ApiVersion = apiVersion;
            this.ClusterConfigurationVersion = userConfig.Version.Version;
            this.Name = userConfig.ClusterName;
        }

        internal virtual void ValidateModel()
        {
            this.Name.ThrowValidationExceptionIfNullOrEmpty("Name");
            this.ApiVersion.ThrowValidationExceptionIfNullOrEmpty("ApiVersion");
            this.ClusterConfigurationVersion.ThrowValidationExceptionIfNullOrEmpty("ClusterConfigurationVersion");
            this.ValidateNodeList();
        }

        internal virtual void ValidateModeDiff(StandAloneInstallerJsonModelBase orignialModel)
        {
            throw new System.NotImplementedException();
        }

        protected abstract StandAloneUserConfig OnGetUserConfig();

        private static StandAloneInstallerJsonModelBase DeserializeJsonConfig(Type type, string json)
        {
            return (StandAloneInstallerJsonModelBase)JsonConvert.DeserializeObject(
                json,
                type,
                new JsonSerializerSettings()
                {
                    DefaultValueHandling = DefaultValueHandling.Populate,
                    PreserveReferencesHandling = PreserveReferencesHandling.None
                });
        }

        private void ValidateNodeList()
        {
            if (this.Nodes == null || this.Nodes.Count == 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAInvalidNodeList);
            }

            var nodeNameDuplicateDetector = new HashSet<string>();
            var faultDomainSize = -1;
            foreach (var node in this.Nodes)
            {
                node.Verify();
                if (nodeNameDuplicateDetector.Contains(node.NodeName))
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                        StringResources.Error_BPADuplicateNodeName,
                        node.NodeName);
                }
                else
                {
                    nodeNameDuplicateDetector.Add(node.NodeName);
                }

                if (faultDomainSize == -1)
                {
                    faultDomainSize = node.FaultDomain.Split('/').Length;
                }
                else
                {
                    if (faultDomainSize != node.FaultDomain.Split('/').Length)
                    {
                        throw new ValidationException(
                            ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                            StringResources.Error_BPAInvalidFD,
                            node.NodeName);
                    }
                }
            }

            var groupNodesOnSameMachine = this.Nodes.GroupBy(n => n.IPAddress).Where(group => group.Count() > 1);

            // Validate nodes on same machine don't use same NodeType, to avoid port collisions.
            bool sameNodeTypeOnSameMachine = groupNodesOnSameMachine
                                             .Any(group => (group.Count() != group.Select(m => m.NodeTypeRef).Distinct().Count()));
            if (sameNodeTypeOnSameMachine)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAJsonNodesCannotReferenceSameNodeType);
            }

            // Validate nodes on same machine with different NodeType, have no port collisions.
            var nodeTypeDictionary = this.GetUserConfig().NodeTypes.ToDictionary(nodeType => nodeType.Name, nodeType => NodeTypeDescriptionGA.ReadFromInternal(nodeType));
            bool sameMachinePortCollision = groupNodesOnSameMachine
                                            .Any(group => this.NodeTypesHavePortCollisions(group.Select(g => nodeTypeDictionary[g.NodeTypeRef])));
            if (sameMachinePortCollision)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAJsonNodesSameMachinePortCollision);
            }
        }

        private bool NodeTypesHavePortCollisions(IEnumerable<NodeTypeDescriptionGA> nodeTypes)
        {
            foreach (var nodeType1 in nodeTypes)
            {
                foreach (var nodeType2 in nodeTypes)
                {
                    if (nodeType1 == nodeType2)
                    {
                        continue;
                    }

                    if (nodeType1.ClientConnectionEndpointPort == nodeType2.ClientConnectionEndpointPort)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "ClientConnectionEndpointPort");
                        return true;
                    }

                    if (nodeType1.HttpGatewayEndpointPort == nodeType2.HttpGatewayEndpointPort)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "HttpGatewayEndpointPort");
                        return true;
                    }

                    if (nodeType1.ReverseProxyEndpointPort.HasValue && nodeType2.ReverseProxyEndpointPort.HasValue)
                    {
                        if (nodeType1.ReverseProxyEndpointPort.Value == nodeType2.ReverseProxyEndpointPort.Value)
                        {
                            SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "ReverseProxyEndpointPort");
                            return true;
                        }
                    }

                    if (nodeType1.LeaseDriverEndpointPort == nodeType2.LeaseDriverEndpointPort)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "LeaseDriverEndpointPort");
                        return true;
                    }

                    if (nodeType1.ClusterConnectionEndpointPort == nodeType2.ClusterConnectionEndpointPort)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "ClusterConnectionEndpointPort");
                        return true;
                    }

                    if (nodeType1.ServiceConnectionEndpointPort == nodeType2.ServiceConnectionEndpointPort)
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "ServiceConnectionEndpointPort");
                        return true;
                    }

                    if (this.PortRangeColliding(nodeType1.ApplicationPorts, nodeType2.ApplicationPorts))
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "ApplicationPorts");
                        return true;
                    }

                    if (this.PortRangeColliding(nodeType1.EphemeralPorts, nodeType2.EphemeralPorts))
                    {
                        SFDeployerTrace.WriteError(StringResources.Error_BPAJsonNodeTypesSameMachinePortCollision, nodeType1.Name, nodeType2.Name, "EphemeralPorts");
                        return true;
                    }
                }
            }

            return false;
        }

        private bool PortRangeColliding(EndpointRangeDescription range1, EndpointRangeDescription range2)
        {
            if (range1 == null || range2 == null)
            {
                return false;
            }

            return range1.StartPort <= range2.EndPort && range1.EndPort >= range2.StartPort;
        }
    }
}