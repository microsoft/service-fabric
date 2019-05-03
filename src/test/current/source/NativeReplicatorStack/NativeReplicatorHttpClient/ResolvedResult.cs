// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using System.Fabric;
    using System.Collections.Generic;

    internal class ResolvedResult
    {
        public ResolvedServicePartition Partition { get; set; }
        public ResolvedServiceEndpoint Endpoint { get; set; }

        public string PrimaryAddress { get; set; }
        public string PrimaryEndpoint { get; set; }
        public IList<string> Endpoints { get; set; }
    }
}