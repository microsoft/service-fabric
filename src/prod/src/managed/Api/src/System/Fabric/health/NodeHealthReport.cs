// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on the node health entity. 
    /// The report is sent to health store with 
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </summary>
    public class NodeHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.NodeHealthReport" /> class.</para>
        /// </summary>
        /// <param name="nodeName">
        /// <para>The node name. Can’t be null or empty.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, 
        /// property, health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Node name can’t be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Health information can’t be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Node name can’t be empty.</para>
        /// </exception>
        public NodeHealthReport(string nodeName, HealthInformation healthInformation)
            : base(HealthReportKind.Node, healthInformation)
        {
            Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();
            this.NodeName = nodeName;
        }

        /// <summary>
        /// <para>Gets the node name.</para>
        /// </summary>
        /// <value>
        /// <para>The node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeNodeHealthReport = new NativeTypes.FABRIC_NODE_HEALTH_REPORT();
            nativeNodeHealthReport.NodeName = pinCollection.AddObject(this.NodeName);
            nativeNodeHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeNodeHealthReport);
        }
    }
}