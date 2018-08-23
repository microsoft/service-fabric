// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the list of nodes targeted by a repair action.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public sealed class NodeRepairTargetDescription : RepairTargetDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeRepairTargetDescription" /> class with 
        /// an empty list of node names.</para>
        /// </summary>
        public NodeRepairTargetDescription()
            : base(RepairTargetKind.Node)
        {
            this.Nodes = new List<string>();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeRepairTargetDescription" /> class that 
        /// contains the given node name as the only element in its node list.</para>
        /// </summary>
        /// <param name="nodeName">
        /// <para>The name of the node to be added to the list.</para>
        /// </param>
        public NodeRepairTargetDescription(string nodeName)
            : base(RepairTargetKind.Node)
        {
            Requires.Argument("nodeName", nodeName).NotNullOrEmpty();

            this.Nodes = new List<string>();
            this.Nodes.Add(nodeName);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeRepairTargetDescription" /> class that 
        /// contains elements copied from the specified collection.</para>
        /// </summary>
        /// <param name="nodeNames">
        /// <para>The collection whose elements are copied to the new list.</para>
        /// </param>
        public NodeRepairTargetDescription(IEnumerable<string> nodeNames)
            : base(RepairTargetKind.Node)
        {
            Requires.Argument("nodeNames", nodeNames).NotNull();

            var nodes = new List<string>();
            nodes.AddRange(nodeNames);
            this.Nodes = nodes;
        }

        /// <summary>
        /// <para>Gets the list of node names.</para>
        /// </summary>
        /// <value>
        /// <para>A list of node names.</para>
        /// </value>
        public IList<string> Nodes { get; private set; }

        internal static unsafe new NodeRepairTargetDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            var nativeList = (NativeTypes.FABRIC_STRING_LIST*)nativeDescriptionPtr;
            var managedList = NativeTypes.FromNativeStringList(*nativeList);

            // TODO avoid extra copy here
            return new NodeRepairTargetDescription(managedList);
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            return NativeTypes.ToNativeStringList(pinCollection, this.Nodes);
        }

        /// <summary>
        /// <para>Converts the value of the current object to a string representation.</para>
        /// </summary>
        /// <returns>
        /// <para>A string representation of the value of <see cref="System.Fabric.Repair.NodeRepairTargetDescription.Nodes" />.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Join(",", this.Nodes);
        }
    }
}