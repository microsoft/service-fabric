// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MCF.Client
{
    using System.Fabric;

    internal class ResolvedResult
    {
        public ResolvedServicePartition Partition { get; set; }
        public ResolvedServiceEndpoint Endpoint { get; set; }
    }
}