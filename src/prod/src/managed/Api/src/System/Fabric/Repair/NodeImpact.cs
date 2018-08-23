// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the expected impact of a repair to a particular node.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public sealed class NodeImpact
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeImpact" /> class.</para>
        /// </summary>
        public NodeImpact()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.NodeImpact" /> class.</para>
        /// </summary>
        /// <param name="nodeName">
        /// <para>The name of the impacted node.</para>
        /// </param>
        /// <param name="impactLevel">
        /// <para>The level of impact expected.</para>
        /// </param>
        public NodeImpact(string nodeName, NodeImpactLevel impactLevel)
        {
            this.NodeName = nodeName;
            this.ImpactLevel = impactLevel;
        }

        /// <summary>
        /// <para>Gets or sets the name of the impacted node.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the impacted node.</para>
        /// </value>
        public string NodeName { get; set; }
        /// <summary>
        /// <para>Gets or sets the level of impact expected.</para>
        /// </summary>
        /// <value>
        /// <para>The level of impact expected.</para>
        /// </value>
        public NodeImpactLevel ImpactLevel { get; set; }

        internal static unsafe NodeImpact CreateFromNative(NativeTypes.FABRIC_REPAIR_NODE_IMPACT nativeImpact)
        {
            var description = new NodeImpact()
            {
                NodeName = NativeTypes.FromNativeString(nativeImpact.NodeName),
                ImpactLevel = (NodeImpactLevel)nativeImpact.ImpactLevel,
            };

            return description;
        }

        /// <summary>
        /// <para>Converts the value of the current object to a string representation.</para>
        /// </summary>
        /// <returns>
        /// <para>A string representation of the node name and impact level.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Concat(this.NodeName, ":", this.ImpactLevel);
        }
    }
}