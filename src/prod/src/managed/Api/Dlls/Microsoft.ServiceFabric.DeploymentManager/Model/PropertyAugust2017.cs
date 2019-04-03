// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class PropertyAugust2017 : PropertyBase
    {
        public PropertyAugust2017()
        {
            this.AddonFeatures = new List<AddonFeature>();
        }

        public SecurityMay2017 Security
        {
            get;
            set;
        }

        public List<NodeTypeDescriptionAugust2017> NodeTypes
        {
            get;
            set;
        }

        [JsonProperty(ItemConverterType = typeof(StringEnumConverter))]
        public List<AddonFeature> AddonFeatures { get; set; }

        [OnDeserialized]
        internal void OnDeserializedCallback(StreamingContext context)
        {
            if (this.AddonFeatures == null)
            {
                this.AddonFeatures = new List<AddonFeature>();
            }
        }

        internal override void Validate(List<NodeDescriptionGA> nodes)
        {
            base.Validate(nodes);
            var referenceNodeTypes = nodes.Select(n => n.NodeTypeRef).Distinct();
            this.ValidateNodeTypes(referenceNodeTypes);
        }

        internal override void UpdateUserConfig(IUserConfig userConfig, List<NodeDescriptionGA> nodes)
        {
            base.UpdateUserConfig(userConfig, nodes);
            this.UpdateNodeTypesIfNeeded(nodes);
            userConfig.NodeTypes = this.NodeTypes.ConvertAll(external => external.ConvertToInternal());
            userConfig.PrimaryNodeTypes = userConfig.NodeTypes.FindAll(nt => nt.IsPrimary);
            userConfig.TotalPrimaryNodeTypeNodeCount = nodes.Count(node => userConfig.PrimaryNodeTypes.FirstOrDefault(nt => nt.Name == node.NodeTypeRef) != null);

            if (this.Security != null)
            {
                userConfig.Security = this.Security.ToInternal();
            }

            userConfig.ReliabilityLevel = !this.TestHookReliabilityLevel.HasValue ? ReliabilityLevelExtension.GetReliabilityLevel(userConfig.TotalPrimaryNodeTypeNodeCount) : this.TestHookReliabilityLevel.Value;
            userConfig.AddonFeatures = this.AddonFeatures;
        }

        internal override void UpdateFromUserConfig(IUserConfig userConfig)
        {
            base.UpdateFromUserConfig(userConfig);
            this.NodeTypes = userConfig.NodeTypes.ConvertAll<NodeTypeDescriptionAugust2017>(
                nodeType => NodeTypeDescriptionAugust2017.ReadFromInternal(nodeType));

            this.Security = new SecurityMay2017();
            if (userConfig.Security != null)
            {               
                this.Security.FromInternal(userConfig.Security);
            }

            if (userConfig.AddonFeatures != null)
            {
                this.AddonFeatures = userConfig.AddonFeatures;
            }
        }

        internal void UpdateNodeTypesIfNeeded(List<NodeDescriptionGA> nodes)
        {
            // 1. If two nodes have the same IP, and one of them is set to be primary node type, we set the ones with the same IP's nodeType all to be primary node type.
            // This adds more flexibility to the customers. They can either define scale Min nodes to be all primary or only one of them to be primary.
            var primaryNodesTypes = this.NodeTypes.Where(nodeType => nodeType.IsPrimary);
            var primaryNodeTypeNodes = nodes.Where(
                                           node => primaryNodesTypes.Any(
                                               primaryNodeType => primaryNodeType.Name == node.NodeTypeRef));
            var nodesThatHaveTheSameIpWithprimaryNodeTypeNodes = nodes.Where(
                        node => primaryNodeTypeNodes.Any(
                            primaryNodeTypeNode =>
                            primaryNodeTypeNode.NodeName != node.NodeName
                            && primaryNodeTypeNode.IPAddress == node.IPAddress));
            var nodeTypesThatNeedToBeSetToPrimary = nodesThatHaveTheSameIpWithprimaryNodeTypeNodes.Select(node => node.NodeTypeRef);

            foreach (var nodeType in this.NodeTypes)
            {
                if (nodeTypesThatNeedToBeSetToPrimary.Contains(nodeType.Name))
                {
                    nodeType.IsPrimary = true;
                }
            }
        }

        private void ValidateNodeTypes(IEnumerable<string> referencedNodeTypes)
        {
            if (this.NodeTypes == null || this.NodeTypes.Count() == 0)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                    StringResources.Error_BPANodeTypesElementMissing);
            }

            var nodeTypeNames = new HashSet<string>();
            foreach (var nodeType in this.NodeTypes)
            {
                nodeType.Verify();
                if (nodeTypeNames.Contains(nodeType.Name))
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                        StringResources.Error_BPANodeTypeNamesOverlapping,
                        nodeType.Name);
                }

                nodeTypeNames.Add(nodeType.Name);
            }

            if (referencedNodeTypes.Any(rnt => !nodeTypeNames.Contains(rnt)))
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPANodeTypeRefIncorrect);
            }
        }
    }
}