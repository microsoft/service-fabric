// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <summary>
    /// Wrapper for Policy Agent's Bond type <see cref="RoleInstanceImpactedByJob"/>
    /// </summary>
    internal class ImpactedRoleInstance
    {
        public ImpactedRoleInstance()
        {
            ImpactTypes = new List<string>();    
        }

        public string Name { get; set; }

        public string UD { get; set; }

        public IList<string> ImpactTypes { get; set; }

        public override string ToString()
        {
            return "{0}, UD: {1}, ImpactTypes: {2}".ToString(Name, UD, string.Join(", ", ImpactTypes));
        }
    }
}