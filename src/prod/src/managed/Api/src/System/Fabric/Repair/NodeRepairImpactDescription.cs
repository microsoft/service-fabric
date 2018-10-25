// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the expected impact of a repair on a set of nodes.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public sealed class NodeRepairImpactDescription : RepairImpactDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeRepairImpactDescription" /> class with 
        /// an empty impact list.</para>
        /// </summary>
        public NodeRepairImpactDescription()
            : this(new List<NodeImpact>())
        {
        }

        private NodeRepairImpactDescription(IList<NodeImpact> impactedNodes)
            : base(RepairImpactKind.Node)
        {
            this.ImpactedNodes = impactedNodes;
        }

        /// <summary>
        /// <para>Gets the list of impacted nodes.</para>
        /// </summary>
        /// <value>
        /// <para>A list of <see cref="System.Fabric.Repair.NodeImpact" /> objects describing the impact of the repair.</para>
        /// </value>
        public IList<NodeImpact> ImpactedNodes { get; private set; }

        internal static unsafe new NodeRepairImpactDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            var nativeDescription = (NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LIST*)nativeDescriptionPtr;
            var nativeItemArray = (NativeTypes.FABRIC_REPAIR_NODE_IMPACT*)nativeDescription->Items;

            List<NodeImpact> impactList = new List<NodeImpact>();

            for (int i = 0; i < nativeDescription->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                impactList.Add(NodeImpact.CreateFromNative(nativeItem));
            }

            NodeRepairImpactDescription description = new NodeRepairImpactDescription(impactList);
            return description;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeArray = new NativeTypes.FABRIC_REPAIR_NODE_IMPACT[this.ImpactedNodes.Count];
            for (int i = 0; i < nativeArray.Length; ++i)
            {
                nativeArray[i].NodeName = pinCollection.AddBlittable(this.ImpactedNodes[i].NodeName);
                nativeArray[i].ImpactLevel = (NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL)this.ImpactedNodes[i].ImpactLevel;
            }

            var nativeDescription = new NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LIST();
            nativeDescription.Count = (uint)nativeArray.Length;
            nativeDescription.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeDescription);
        }

        /// <summary>
        /// <para>Converts the value of the current object to a string representation.</para>
        /// </summary>
        /// <returns>
        /// <para>A string representation of the value of <see cref="System.Fabric.Repair.NodeRepairImpactDescription.ImpactedNodes" />.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Join(",", this.ImpactedNodes);
        }
    }
}