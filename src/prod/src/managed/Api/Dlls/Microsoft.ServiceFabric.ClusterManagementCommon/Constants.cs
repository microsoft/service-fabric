// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{ 
    internal class Constants
    {
        public static readonly string ProviderNamespace = "Microsoft.ServiceFabric";
        public static readonly string ResourceType = ProviderNamespace + "/clusters";

        /// <summary>
        /// Request URI parameter to filter on the given expression.
        /// </summary>
        public static readonly string QueryFilter = "Filter";
    }
}