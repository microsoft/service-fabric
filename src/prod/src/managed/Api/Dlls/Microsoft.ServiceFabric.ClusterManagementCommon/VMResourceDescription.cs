// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;

    public class VMResourceDescription
    {
        public string Name { get; set; }

        public string NodeTypeRef { get; set; }

        [Required]
        [Range(1, int.MaxValue)]
        public int VMInstanceCount { get; set; }

        public bool IsVMSS { get; set; }

        public PaaSRoleType ToPaaSRoleType()
        {
            return new PaaSRoleType()
            {
                RoleName = this.Name,
                NodeTypeRef = this.NodeTypeRef,
                RoleNodeCount = this.VMInstanceCount
            };
        }
    }
}