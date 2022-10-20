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
    public class WinFabricRepairImpactDescription
    {
        [DataMember(Name = "Kind", IsRequired = true)]
        public WinFabricRepairImpactKind Kind { get; set; }
    }

    [Serializable]
    [DataContract]
    public class WinFabricNodeRepairImpactDescription : WinFabricRepairImpactDescription
    {
        [DataMember(Name = "ImpactedNodes", IsRequired = true)]
        public IList<WinFabricNodeImpact> ImpactedNodes { get; set; }

        public WinFabricNodeRepairImpactDescription()
        {
            Kind = WinFabricRepairImpactKind.Node;
        }
    }

    [Serializable]
    [DataContract]
    public class WinFabricNodeImpact
    {
        [DataMember(Name = "NodeName", IsRequired = true)]
        public string NodeName { get; set; }

        [DataMember(Name = "ImpactLevel", IsRequired = true)]
        public WinFabricNodeImpactLevel ImpactLevel { get; set; }
    }

    [Serializable]
    public enum WinFabricRepairImpactKind
    {
        Invalid = 0,
        Node = 1
    }

    [Serializable]
    public enum WinFabricNodeImpactLevel
    {
        Invalid = 0,
        None = 1,
        Restart = 2,
        RemoveData = 3,
        RemoveNode = 4,
    }
}

#pragma warning restore 1591