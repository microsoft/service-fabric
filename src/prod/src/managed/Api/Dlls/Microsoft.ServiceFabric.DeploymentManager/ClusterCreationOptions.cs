// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    using System;

    /// <summary>
    /// Cluster Creation options used when invoking Standalone cluster deployment.
    /// </summary>
    [Flags]
    public enum ClusterCreationOptions : int
    {
        /// <summary>
        /// <para>Default cluster creation option: In the case of failure during node configuration, Fabric will be cleaned from all affected machines.</para>
        /// </summary>
        None = 0,

        /// <summary>
        /// <para>No recovery action will be performed in case of errors. Use if any failures will be manually resolved.</para>
        /// </summary>
        OptOutCleanupOnFailure = 1,

        /// <summary>
        /// <para>Force delete fabric data root from previous installations, if any.</para>
        /// </summary>
        Force = 2
    }
}