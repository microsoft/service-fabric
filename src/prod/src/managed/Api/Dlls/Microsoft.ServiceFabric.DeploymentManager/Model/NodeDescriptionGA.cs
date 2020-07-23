// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    public class NodeDescriptionGA
    {
        public NodeDescriptionGA()
        {
        }

        public string NodeName
        {
            get;
            set;
        }

        public string NodeTypeRef { get; set; }

        // ReSharper disable once InconsistentNaming - Backwards Compatibility.
        public string IPAddress { get; set; }

        public string FaultDomain { get; set; }

        public string UpgradeDomain { get; set; }

        internal static NodeDescriptionGA ReadFromInternal(NodeDescription nodeDescription)
        {
            return new NodeDescriptionGA
            {
                NodeName = nodeDescription.NodeName,
                IPAddress = nodeDescription.IPAddress,
                FaultDomain = nodeDescription.FaultDomain,
                UpgradeDomain = nodeDescription.UpgradeDomain,
                NodeTypeRef = nodeDescription.NodeTypeRef
            };
        }

        internal NodeDescription ConvertToInternal()
        {
            return new NodeDescription
            {
                NodeName = this.NodeName,
                IPAddress = this.IPAddress,
                FaultDomain = this.FaultDomain,
                UpgradeDomain = this.UpgradeDomain,
                NodeTypeRef = this.NodeTypeRef
            };
        }

        internal void Verify()
        {
            this.NodeName.ThrowValidationExceptionIfNullOrEmpty("Node/NodeName");
            this.FaultDomain.ThrowValidationExceptionIfNullOrEmpty("Node/FaultDomain");
            this.IPAddress.ThrowValidationExceptionIfNullOrEmpty("Node/IPAddress");
            this.NodeTypeRef.ThrowValidationExceptionIfNullOrEmpty("Node/NodeTypeRef");
            this.UpgradeDomain.ThrowValidationExceptionIfNullOrEmpty("Node/UpgradeDomain");

            if (!FabricValidatorUtility.IsValidIPAddressOrFQDN(this.IPAddress))
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, 
                    StringResources.Error_BPAInvalidIPAddress, 
                    this.IPAddress); 
            }

            if (!FabricValidatorUtility.IsValidFileName(this.NodeName))
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, 
                    StringResources.Error_BPAInvalidNodeName,
                    this.NodeName);
            }
        }
    }
}