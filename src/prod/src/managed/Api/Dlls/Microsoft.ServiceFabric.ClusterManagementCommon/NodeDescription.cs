// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;

    public class NodeDescription
    {
        private string nodeName;

        public NodeDescription()
        {
            this.NodeInstance = int.MaxValue;
        }

        public string NodeName 
        {
            get
            {
                return this.nodeName;
            }

            set
            {
                this.nodeName = value;

                // Incase of VMSS the node name format will be _<VMSSName>_<NodeInstance>
                // Parse the name to get the NodeInstance
                var lastIndex = value.LastIndexOf('_');
                if (lastIndex != -1)
                {
                    var instanceStr = value.Substring(lastIndex + 1);
                    int nodeInstance;
                    if (int.TryParse(instanceStr, out nodeInstance))
                    {
                        this.NodeInstance = nodeInstance;
                    }
                }
            }
        }

        public int NodeInstance { get; set; }

        public string RoleName { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string NodeTypeRef { get; set; }

        [Required(AllowEmptyStrings = false)]

        // ReSharper disable once InconsistentNaming - Backwards Compatibility.
        public string IPAddress { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string FaultDomain { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string UpgradeDomain { get; set; }

        public FabricNodeType ToFabricNodeType()
        {
            return new FabricNodeType()
            {
                NodeName = this.NodeName,
                NodeTypeRef = this.NodeTypeRef,
                IPAddressOrFQDN = this.IPAddress,
                FaultDomain = this.FaultDomain,
                UpgradeDomain = this.UpgradeDomain,
            };
        }

        public override string ToString()
        {
            return string.Format(
                "[NodeName={0}, RoleName={1}, NodeTypeRef={2}, IPAddress={3}, FaultDomain={4}, UpgradeDomain={5}",
                this.NodeName,
                this.RoleName,
                this.NodeTypeRef,
                this.IPAddress,
                this.FaultDomain,
                this.UpgradeDomain);
        }

        public override bool Equals(object other)
        {
            var otherDescription = (NodeDescription)other;
            if (this.NodeName != otherDescription.NodeName)
            {
                return false;
            }

            if (this.RoleName != otherDescription.RoleName)
            {
                return false;
            }

            if (this.NodeTypeRef != otherDescription.NodeTypeRef)
            {
                return false;
            }

            if (this.IPAddress != otherDescription.IPAddress)
            {
                return false;
            }

            if (this.FaultDomain != otherDescription.FaultDomain)
            {
                return false;
            }

            if (this.UpgradeDomain != otherDescription.UpgradeDomain)
            {
                return false;
            }

            return true;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.NodeName.GetHashCode();
            hash = (13 * hash) + this.RoleName.GetHashCode();
            hash = (13 * hash) + this.NodeTypeRef.GetHashCode();
            hash = (13 * hash) + this.IPAddress.GetHashCode();
            hash = (13 * hash) + this.FaultDomain.GetHashCode();
            hash = (13 * hash) + this.UpgradeDomain.GetHashCode();
            return hash;
        }
    }
}