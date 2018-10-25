// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;

    /// <summary>
    /// <para>Represents the data structure that holds a summary of all metrics and their load on a certain node.</para>
    /// </summary>
    public class NodeLoadInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.NodeLoadInformation" /> class.</para>
        /// </summary>
        public NodeLoadInformation()
        {
        }

        /// <summary>
        /// <para>Gets the name of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the node.</para>
        /// </value>
        public string NodeName { get; set; }

        /// <summary>
        /// <para>Gets the list that contains metrics and their load information on a node.</para>
        /// </summary>
        /// <value>
        /// <para>The list that contains metrics and their load information on a node.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeLoadMetricInformation)]
        public IList<NodeLoadMetricInformation> NodeLoadMetricInformationList { get; internal set; }

        internal static unsafe NodeLoadInformation CreateFromNative(
            NativeClient.IFabricGetNodeLoadInformationResult nativeNodeLoadInformation)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_NODE_LOAD_INFORMATION*)nativeNodeLoadInformation.get_NodeLoadInformation());
            GC.KeepAlive(nativeNodeLoadInformation);

            return retval;
        }

        internal static unsafe NodeLoadInformation CreateFromNative(
            NativeTypes.FABRIC_NODE_LOAD_INFORMATION nativeLoadInformation)
        {
            IList<NodeLoadMetricInformation> nodeLoadMetricInformationList;

            if (nativeLoadInformation.NodeLoadMetricInformationList == IntPtr.Zero)
            {
                nodeLoadMetricInformationList = new List<NodeLoadMetricInformation>();
            }
            else
            {
                nodeLoadMetricInformationList = NodeLoadMetricInformation.CreateFromNativeList(
                    (NativeTypes.FABRIC_NODE_LOAD_METRIC_INFORMATION_LIST*)nativeLoadInformation.NodeLoadMetricInformationList);
            }

            return new NodeLoadInformation
            {
                NodeName = NativeTypes.FromNativeString(nativeLoadInformation.NodeName),
                NodeLoadMetricInformationList = nodeLoadMetricInformationList
            };
        }
    }
}