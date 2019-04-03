// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    public class WinFabricRepairTargetDescription
    {
        [DataMember(Name = "Kind", IsRequired = true)]
        public WinFabricRepairTargetKind Kind { get; set; }
    }

    [Serializable]
    [DataContract]
    public class WinFabricNodeRepairTargetDescription : WinFabricRepairTargetDescription
    {
        [DataMember(Name = "Nodes", IsRequired = true)]
        public IList<string> Nodes { get; set; }

        public WinFabricNodeRepairTargetDescription()
        {
            Kind = WinFabricRepairTargetKind.Node;
        }
    }

    [Serializable]
    public enum WinFabricRepairTargetKind
    {
        Invalid = 0,
        Node = 1
    }
}

#pragma warning restore 1591