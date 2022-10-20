// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    public static class Utilities
    {
        public static string GenerateEndpoint(
            HTTPClient.ClusterEnvironment clusterEnv,
            string gatewayAddress, 
            string fqdn,
            bool isPrimary)
        {
            string replica = isPrimary ? "PrimaryReplica" : "RandomSecondaryReplica";

            if (clusterEnv == HTTPClient.ClusterEnvironment.Windows)
            {
                return string.Format("http://{0}/{1}?TargetReplicaSelector={2}", gatewayAddress, fqdn, replica);
            }
            else
            {
                return string.Format("http://{0}?{1}&TargetReplicaSelector={2}", gatewayAddress.Split(':')[0] + ":8505", "ServiceInstanceName=" + fqdn, replica);
            }
        }

        public static string GenerateEndpoint(
            HTTPClient.ClusterEnvironment clusterEnv,
            string gatewayAddress,
            string fqdn,
            string partitionKey,
            string partitionKind,
            bool isPrimary)
        {
            string sufix = string.Format("PartitionKey={0}&PartitionKind={1}&TargetReplicaSelector={2}",
                partitionKey, partitionKind, isPrimary ? "PrimaryReplica" : "RandomSecondaryReplica");

            if (clusterEnv == HTTPClient.ClusterEnvironment.Windows)
            {
                return string.Format("http://{0}/{1}?{2}", gatewayAddress, fqdn, sufix);
            }
            else
            {
                return string.Format("http://{0}?{1}&{2}", gatewayAddress.Split(':')[0] + ":8505", "ServiceInstanceName=" + fqdn, sufix);
            }
        }
    }
}