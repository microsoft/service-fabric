// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NativeReplicatorClientWorkload
{
    public static class Utility
    {
        public static string GenerateEndpoint(string gatewayAddress, string appName, string serviceName, string partitionKey)
        {
            return $"http://{gatewayAddress}/{appName}/{serviceName}?PartitionKey={partitionKey}&PartitionKind=Int64Range";
        }
    }
}