// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class PropertyGA : PropertyBase
    {
        public SecurityGA Security
        {
            get;
            set;
        }

        public virtual List<NodeTypeDescriptionGA> NodeTypes
        {
            get;
            set;
        }

        [JsonConverter(typeof(StringEnumConverter))]
        public ReliabilityLevel ReliabilityLevel
        {
            get;
            set;
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

            userConfig.ReliabilityLevel = this.ReliabilityLevel;
        }

        internal override void UpdateFromUserConfig(IUserConfig userConfig)
        {
            base.UpdateFromUserConfig(userConfig);
            this.Security = new SecurityGA();
            if (userConfig.Security != null)
            {                
                this.Security.FromInternal(userConfig.Security);
            }

            this.ReliabilityLevel = userConfig.ReliabilityLevel;
        }

        internal override void Validate(List<NodeDescriptionGA> nodes)
        {
            base.Validate(nodes);
            var referenceNodeTypes = nodes.Select(n => n.NodeTypeRef).Distinct();
            this.ValidateNodeTypes(referenceNodeTypes);
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