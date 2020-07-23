// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Query
{
    using System.Collections.Generic;
    using System.Fabric.Query;

    /// <summary>
    /// <para>Represents the list of <see cref="ComposeDeploymentStatusResultItem" /> retrieved by calling 
    /// System.Fabric.FabricClient.QueryClient.GetComposeDeploymentStatusPagedListAsync(Microsoft.ServiceFabric.Preview.Client.Description.ComposeDeploymentStatusQueryDescription).</para>
    /// </summary>
    public sealed class ComposeDeploymentStatusList : PagedList<ComposeDeploymentStatusResultItem>
    {
        /// <summary>
        /// <para>Creates a docker compose deployment list. </para>
        /// </summary>
        public ComposeDeploymentStatusList() : base()
        {
        }

        internal ComposeDeploymentStatusList(IList<ComposeDeploymentStatusResultItem> list) : base(list)
        {
        }
    }
}