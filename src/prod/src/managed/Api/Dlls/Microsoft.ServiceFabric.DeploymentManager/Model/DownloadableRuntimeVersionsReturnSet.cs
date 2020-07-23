// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    /// <summary>
    /// Runtime versions return set when retrieving the possible runtime versions from goal state file.
    /// </summary>
    public enum DownloadableRuntimeVersionReturnSet
    {
        /// <summary>
        /// <para>Default runtime versions to be returned. All currently supported versions</para>
        /// </summary>
        Supported = 0,

        /// <summary>
        /// <para>All runtime versions returned from the goal state file</para>
        /// </summary>
        All = 1,

        /// <summary>
        /// <para>Only the runtime version marked as IsGoalPackage:True will be returned.</para>
        /// </summary>
        Latest = 2
    }
}