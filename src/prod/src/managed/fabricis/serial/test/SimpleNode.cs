// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    internal sealed class SimpleNode : INode
    {
        public string NodeName { get; set; }

        public DateTimeOffset? NodeUpTimestamp { get; set; }

        public override string ToString()
        {
            return "{0}, NodeUpTimestamp: {1}".ToString(NodeName, NodeUpTimestamp != null ? NodeUpTimestamp.Value.ToString("o") : "<null>");
        }
    }
}