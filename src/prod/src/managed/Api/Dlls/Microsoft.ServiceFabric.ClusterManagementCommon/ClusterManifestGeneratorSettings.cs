// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;

    public class ClusterManifestGeneratorSettings
    {
        public int FDCount { get; set; }

        public int UDCount { get; set; }

        public int MinAllowedPortNumber { get; set; }

        public int MaxAllowedPortNumber { get; set; }

        public int StartReservedPortNumber { get; set; }

        public int EndReservedPortNumber { get; set; }

        public int MinDynamicPortCount { get; set; }

        public bool AllowUnprotectedDiagnosticsStorageAccountKeys { get; set; }

        public int MaxClusterSize { get; set; }

        public int MinClusterSize { get; set; }

        public int DataDeletionAgeInDaysForDiagnosticsDisabledClusters { get; set; }        

        public IDictionary<string, string> SystemServicesForScale { get; set; }

        public int ClusterConnectionEndpointPort { get; set; }

        public int LeaseDriverEndpointPort { get; set; }

        public int ServiceConnectionEndpointPort { get; set; }
    }
}