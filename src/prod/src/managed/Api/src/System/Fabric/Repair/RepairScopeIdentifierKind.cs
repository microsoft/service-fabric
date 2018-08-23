// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the kind of the <see cref="System.Fabric.Repair.RepairScopeIdentifier" />.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    public enum RepairScopeIdentifierKind
    {
        /// <summary>
        /// <para>The repair scope identifier is not valid or is of an unknown type.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND.FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_INVALID,
        /// <summary>
        /// <para>The repair scope identifier represents the entire cluster.</para>
        /// </summary>
        Cluster = NativeTypes.FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND.FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER,
    }
}