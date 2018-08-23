// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// Specifies the application capacity for one metric.
    /// </summary>
    /// <seealso cref="System.Fabric.Description.ApplicationDescription"/>
    /// <seealso cref="System.Fabric.Description.ApplicationUpdateDescription"/>
    public sealed class ApplicationMetricDescription
    {
        /// <summary>
        /// Gets or sets the name of the metric.
        /// </summary>
        /// <value>
        /// The name of the metric.
        /// </value>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the node reservation capacity for Service Fabric application.
        /// </summary>
        /// <value>
        /// <para>
        /// Specifies the amount of Metric Load which is reserved on nodes which have instances of this Application.
        /// If <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/> is specified, then
        /// the product of these values will be the Capacity reserved in the cluster for the Application.
        /// </para>
        /// <para>
        /// If set to zero, no capacity is reserved for this metric.
        /// </para>
        /// <para>
        /// When setting application capacity (<see cref="System.Fabric.Description.ApplicationDescription"/>) or when updating
        /// application capacity ((<see cref="System.Fabric.Description.ApplicationUpdateDescription"/>) this value must be smaller than
        /// or equal to <see cref="System.Fabric.Description.ApplicationMetricDescription.MaximumNodeCapacity"/> for each metric.
        /// </para>
        /// </value>
        public long NodeReservationCapacity { get; set; }

        /// <summary>
        /// Gets or sets the maximum node capacity for Service Fabric application.
        /// </summary>
        /// <value>
        /// <para>
        /// Specifies the Maximum Load for an instance of this Application on a single Node.
        /// Even if Capacity of the node is greater than this value, Service Fabric will limit the
        /// total load of services within the Application on each node to this value.
        /// </para>
        /// <para>If set to zero, capacity for this metric is unlimited on each node.</para>
        /// <para>
        /// When creating a new application with application capacity defined, the product of <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/>
        /// and this value must always be smaller than or equal to <see cref="System.Fabric.Description.ApplicationMetricDescription.TotalApplicationCapacity"/>.
        /// </para>
        /// <para>
        /// When updating existing application with application capacity, the product of <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/>
        /// and this value must always be smaller than or equal to <see cref="System.Fabric.Description.ApplicationMetricDescription.TotalApplicationCapacity"/>.
        /// </para>
        /// </value>
        public long MaximumNodeCapacity { get; set; }

        /// <summary>
        /// Gets or sets the total metric capacity for Service Fabric application.
        /// </summary>
        /// <value>
        /// <para>
        /// Specifies the total metric capacity for this Application in the Cluster.
        /// Service Fabric will try to limit the sum of loads of services within the Application to this value.
        /// </para>
        /// <para>
        /// When creating a new application with application capacity defined, the product of <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/>
        /// and <see cref="System.Fabric.Description.ApplicationMetricDescription.MaximumNodeCapacity"/> must always be smaller than or equal to this value.
        /// </para>
        /// <para>
        /// When creating a new application with application capacity defined, the product of <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/>
        /// and <see cref="System.Fabric.Description.ApplicationMetricDescription.MaximumNodeCapacity"/> must always be smaller than or equal to this value.
        /// </para>
        /// </value>
        public long TotalApplicationCapacity { get; set; }

        internal static void Validate(ApplicationMetricDescription description, long maximumNodes = 0)
        {
            Requires.Argument<string>("Name", description.Name).NotNullOrWhiteSpace();

            // This check is needed because managed type is long, and native type is UINT
            Requires.CheckUInt32ArgumentLimits(description.NodeReservationCapacity, "NodeReservationCapacity");
            Requires.CheckUInt32ArgumentLimits(description.MaximumNodeCapacity, "MaximumNodeCapacity");
            Requires.CheckUInt32ArgumentLimits(description.TotalApplicationCapacity, "TotalApplicationCapacity");

            if (description.NodeReservationCapacity > description.MaximumNodeCapacity)
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ReservationCapacityGreaterThanMaximumNodeCapacity,
                        description.NodeReservationCapacity,
                        description.MaximumNodeCapacity));
            }

            if (maximumNodes * description.MaximumNodeCapacity > description.TotalApplicationCapacity)
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_MaximumCapacityGreaterThanTotalCapacity,
                        description.MaximumNodeCapacity,
                        maximumNodes,
                        description.TotalApplicationCapacity));
            }
        }

        internal void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_APPLICATION_METRIC_DESCRIPTION nativeDescription)
        {
            nativeDescription.Name = pinCollection.AddObject(this.Name);
            // Validation makes sure that these values are >= 0
            nativeDescription.NodeReservationCapacity = checked((uint)this.NodeReservationCapacity);
            nativeDescription.MaximumNodeCapacity = checked((uint)this.MaximumNodeCapacity);
            nativeDescription.TotalApplicationCapacity = checked((uint)this.TotalApplicationCapacity);
            nativeDescription.Reserved = IntPtr.Zero;
        }

        internal static ApplicationMetricDescription CreateFromNative(NativeTypes.FABRIC_APPLICATION_METRIC_DESCRIPTION nativeDescription)
        {
            return new ApplicationMetricDescription()
            {
                Name = NativeTypes.FromNativeString(nativeDescription.Name),
                NodeReservationCapacity = checked((long)nativeDescription.NodeReservationCapacity),
                MaximumNodeCapacity = checked((long)nativeDescription.MaximumNodeCapacity),
                TotalApplicationCapacity = checked((long)nativeDescription.TotalApplicationCapacity)
            };
        }
    }
}