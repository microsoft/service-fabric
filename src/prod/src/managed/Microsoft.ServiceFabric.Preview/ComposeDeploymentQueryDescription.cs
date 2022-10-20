// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the query input used by System.Fabric.FabricClient.ComposeDeploymentClient.GetComposeDeploymentStatusPagedListAsync(Microsoft.ServiceFabric.Preview.Client.Description.ComposeDeploymentStatusQueryDescription) .</para>
    /// </summary>
    public sealed class ComposeDeploymentStatusQueryDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="ComposeDeploymentStatusQueryDescription" /> class.</para>
        /// </summary>
        public ComposeDeploymentStatusQueryDescription()
        {
            DeploymentNameFilter = null;
            ContinuationToken = null;
            MaxResults = 0;
        }

        /// <summary>
        /// <para>Gets or sets the name of compose deployment to query for.</para>
        /// </summary>
        /// <value>
        /// <para>The name of compose deployment to query for.</para>
        /// </value>
        /// <remarks>
        /// <para> If DeploymentNameFilter is not specified, all compose deployments are returned.</para>
        /// </remarks>
        public string DeploymentNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the token that can be used by queries to get the next page.</para>
        /// </summary>
        /// <value>
        /// <para>The token that can be used by queries to get the next page.</para>
        /// </value>
        /// <remarks>
        /// <para>ContinuationToken is received by a previous query.</para>
        /// </remarks>
        public string ContinuationToken { get; set; }

        /// <summary>
        /// Gets or sets the max number of Microsoft.ServiceFabric.Preview.Client.Query.ComposeDeploymentStatusResultItems that can be returned per page.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If this value is not set, it is not used to limit the number of returned Microsoft.ServiceFabric.Preview.Client.Query.ComposeDeploymentStatusResultItems.
        /// </para>
        /// </remarks>
        public long MaxResults { get; set; }

        internal System.Fabric.Description.ComposeDeploymentStatusQueryDescriptionWrapper ToWrapper()
        {
            return new System.Fabric.Description.ComposeDeploymentStatusQueryDescriptionWrapper()
            {
                DeploymentNameFilter = this.DeploymentNameFilter,
                ContinuationToken = this.ContinuationToken,
                MaxResults = this.MaxResults
            };
        }
    }
}