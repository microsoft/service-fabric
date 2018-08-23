// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;

    internal sealed class ClusterRepairScopeIdentifier : RepairScopeIdentifier
    {
        public ClusterRepairScopeIdentifier()
            : base(RepairScopeIdentifierKind.Cluster)
        {
        }

        internal static unsafe new ClusterRepairScopeIdentifier CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            // Ignore value, should be null
            return new ClusterRepairScopeIdentifier();
        }

        public override string ToString()
        {
            return "Cluster";
        }
    }
}