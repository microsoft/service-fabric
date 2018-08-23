// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents data structure that contains load information for a certain metric on a node.</para>
    /// </summary>
    public sealed class NodeLoadMetricInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.NodeLoadMetricInformation" /> class.</para>
        /// </summary>
        public NodeLoadMetricInformation()
        {
        }

        /// <summary>
        /// <para>Gets the name of the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the metric.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Gets the total capacity on the node for the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The total capacity on the node for the metric.</para>
        /// </value>
        public long NodeCapacity { get; internal set; }

        /// <summary>
        /// <para>Gets the current load on the node for a metric.</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.NodeLoadMetricInformation.CurrentNodeLoad"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The current load on the node for a metric.</para>
        /// </value>
        public long NodeLoad { get; internal set; }

        /// <summary>
        /// <para>>Gets the current load on the node for a metric.</para>
        /// </summary>
        /// <value>
        /// <para>The current load on the node for a metric.</para>
        /// </value>
        public double CurrentNodeLoad { get; internal set; }

        /// <summary>
        /// <para>Gets the remaining capacity on the node for the metric.</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.NodeLoadMetricInformation.NodeCapacityRemaining"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity on the node for the metric.</para>
        /// </value>
        public long NodeRemainingCapacity { get; internal set; }


        /// <summary>
        /// <para>Gets the remaining capacity on the node for the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity on the node for the metric.</para>
        /// </value>
        public double NodeCapacityRemaining { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates whether there is a capacity violation for the metric on the given node.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if there is a capacity violation for the metric on the given node; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsCapacityViolation { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates the capacity which is not reserved by NodeBufferPercentage.</para>
        /// </summary>
        /// <value>
        /// <para>The capacity which is not reserved by NodeBufferPercentage for this metric on the node.</para>
        /// </value>
        public long NodeBufferedCapacity { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates the remaining capacity which is not reserved by NodeBufferPercentage.</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.NodeLoadMetricInformation.BufferedNodeCapacityRemaining"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity which is not reserved by NodeBufferPercentage for this metric on the node.</para>
        /// </value>
        public long NodeRemainingBufferedCapacity { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates the remaining capacity which is not reserved by NodeBufferPercentage.</para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity which is not reserved by NodeBufferPercentage for this metric on the node.</para>
        /// </value>
        public double BufferedNodeCapacityRemaining { get; internal set; }

        internal static unsafe NodeLoadMetricInformation CreateFromNative(NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION nativeResultItem)
        {
            long nodeBufferedCapacity = 0;
            long nodeRemainingBufferedCapacity = 0;

            // double values for nodeLoad, remaining capacity and remaining buffered capacity
            double currentNodeLoad = 0;
            double nodeCapacityRemaining = 0;
            double bufferedNodeCapacityRemaining = 0;

            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_EX1* ext1 =
                    (NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_EX1*)nativeResultItem.Reserved;

                nodeBufferedCapacity = ext1->NodeBufferedCapacity;
                nodeRemainingBufferedCapacity = ext1->NodeRemainingBufferedCapacity;

                if (ext1->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_EX2* ext2 =
                    (NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_EX2*) ext1->Reserved;
                    currentNodeLoad = ext2->CurrentNodeLoad;
                    nodeCapacityRemaining = ext2->NodeCapacityRemaining;
                    bufferedNodeCapacityRemaining = ext2->BufferedNodeCapacityRemaining;
                }
            }

            return new NodeLoadMetricInformation
            {
                Name = NativeTypes.FromNativeString(nativeResultItem.Name),
                NodeCapacity = nativeResultItem.NodeCapacity,
                NodeLoad = nativeResultItem.NodeLoad,
                NodeRemainingCapacity = nativeResultItem.NodeRemainingCapacity,
                IsCapacityViolation = NativeTypes.FromBOOLEAN(nativeResultItem.IsCapacityViolation),
                NodeBufferedCapacity = nodeBufferedCapacity,
                NodeRemainingBufferedCapacity = nodeRemainingBufferedCapacity,
                CurrentNodeLoad = currentNodeLoad,
                NodeCapacityRemaining = nodeCapacityRemaining,
                BufferedNodeCapacityRemaining = bufferedNodeCapacityRemaining
            };
        }

        internal static unsafe IList<NodeLoadMetricInformation> CreateFromNativeList(NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_LIST* nativeResultList)
        {
            var rv = new List<NodeLoadMetricInformation>();

            var nativeArray = (NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION*)nativeResultList->Items;
            for (int i = 0; i < nativeResultList->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(NodeLoadMetricInformation.CreateFromNative(nativeItem));
            }

            return rv;
        }
    }
}