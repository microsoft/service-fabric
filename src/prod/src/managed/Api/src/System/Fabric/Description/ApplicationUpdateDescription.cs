// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Globalization;
using System.Fabric.Common;
using System.Fabric.Interop;
using System.Fabric.Strings;

namespace System.Fabric.Description
{
    /// <summary>
    /// Describes an update of application capacity that will be updated using
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UpdateApplicationAsync(ApplicationUpdateDescription)"/>
    /// </summary>
    public sealed class ApplicationUpdateDescription
    {
        /// <summary>
        /// Creates a new instance of <see cref="System.Fabric.Description.ApplicationUpdateDescription"/>.
        /// </summary>
        /// <param name="applicationName">URI of the application instance name.</param>
        public ApplicationUpdateDescription(Uri applicationName) 
        {
            this.ApplicationName = applicationName;
            this.RemoveApplicationCapacity = false;
        }

        /// <summary>
        /// Creates a new instance of <see cref="System.Fabric.Description.ApplicationUpdateDescription"/> with application capacity parameters.
        /// </summary>
        /// <param name="applicationName">URI of the application instance name.</param>
        /// <param name="removeApplicationCapacity">
        /// Indicates if application capacity should be removed
        /// (see <see cref="System.Fabric.Description.ApplicationUpdateDescription.RemoveApplicationCapacity"/>).
        /// </param>
        /// <param name="minimumNodes">
        /// Minimum number of nodes
        /// (see <see cref="System.Fabric.Description.ApplicationUpdateDescription.MinimumNodes"/>).
        /// </param>
        /// <param name="maximumNodes">
        /// Maximum number of nodes
        /// (see <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/>)
        /// </param>
        /// <param name="metrics">
        /// Application capacity metrics
        /// (see <see cref="System.Fabric.Description.ApplicationUpdateDescription.Metrics"/>)
        /// </param>
        public ApplicationUpdateDescription(Uri applicationName,
            bool removeApplicationCapacity,
            long minimumNodes,
            long maximumNodes,
            IList<ApplicationMetricDescription> metrics) 
        {
            this.ApplicationName = applicationName;
            this.RemoveApplicationCapacity = removeApplicationCapacity;
            this.MinimumNodes = minimumNodes;
            this.MaximumNodes = maximumNodes;
            this.Metrics = metrics;
        }

        /// <summary>
        /// <para>Gets or sets the URI name of the application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// Gets or sets the RemoveApplicationCapacity flag.
        /// </summary>
        /// <value>
        /// Used to clear all parameters related to Application Capacity for this application.
        /// It is not possible to specify this parameter together with other Application Capacity parameters.
        /// </value>
        public bool RemoveApplicationCapacity { get; set; }

        /// <summary>
        /// Gets or sets the maximum number of nodes where this application can be instantiated.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes this application is allowed to span. Default value is zero.
        /// If it is zero, Application can be placed on any number of nodes in the cluster.
        /// If this parameter is not specified when updating an application, then the maximum number of nodes remains unchanged.
        /// </para>
        /// <para>
        /// If this parameter is smaller than <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/> an
        /// <see cref="System.ArgumentException"/> will be thrown when <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UpdateApplicationAsync(System.Fabric.Description.ApplicationUpdateDescription)" />
        /// is called.
        /// </para>
        /// </value>
        public long? MaximumNodes { get; set; }

        /// <summary>
        /// Gets or sets the minimum number of nodes.
        /// </summary>
        /// <value>
        /// <para>
        /// Number of nodes where Service Fabric will reserve Capacity in the cluster for this Application to be placed.
        /// Note that this does not mean that the Application is guaranteed to have replicas on all those nodes.
        /// </para>
        /// <para>
        /// If this parameter is set to zero, no capacity will be reserved. If this parameter is not set when updating application then the minimum number of nodes remains unchanged.
        /// </para>
        /// <para>
        /// If this parameters is greater than <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/> and if both parameters are specified then an
        /// <see cref="System.ArgumentException"/> will be thrown when <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UpdateApplicationAsync(System.Fabric.Description.ApplicationUpdateDescription)" />
        /// is called.
        /// </para>
        /// </value>
        public long? MinimumNodes { get; set; }

        /// <summary>
        /// Gets or sets the list of metrics for which the application capacity is defined.
        /// </summary>
        /// <value>
        /// Specifies the metric Capacity of the Application. Capacity is specified for each metric using
        /// <see cref="System.Fabric.Description.ApplicationMetricDescription"/>.
        /// If this parameter is not set, then application capacity metrics will remain unchanged when updating application.
        /// </value>
        public IList<ApplicationMetricDescription> Metrics;

        internal static void Validate(ApplicationUpdateDescription description)
        {
            Requires.Argument<Uri>("ApplicationName", description.ApplicationName).NotNull();
            if (description.RemoveApplicationCapacity && (description.MinimumNodes.HasValue || description.MaximumNodes.HasValue || description.Metrics != null))
            {
                throw new ArgumentException(StringResources.Error_RemoveApplicationCapacity);
            }

            // This check is needed because managed type is long, and native type is UINT
            if (description.MinimumNodes.HasValue)
            {
                Requires.CheckUInt32ArgumentLimits(description.MinimumNodes.Value, "MinimumNodes");
            }

            if (description.MaximumNodes.HasValue)
            {
                Requires.CheckUInt32ArgumentLimits(description.MaximumNodes.Value, "MaximumNodes");
            }

            if (description.MinimumNodes.HasValue 
                && description.MaximumNodes.HasValue
                && (description.MinimumNodes.Value > description.MaximumNodes.Value))
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_MinimumNodesGreaterThanMaximumNodes,
                        description.MinimumNodes.Value,
                        description.MaximumNodes.Value
                ));
            }

            if (description.Metrics != null)
            {
                foreach (var metric in description.Metrics)
                {
                    if (description.MaximumNodes.HasValue)
                    {
                        ApplicationMetricDescription.Validate(metric, description.MaximumNodes.Value);
                    }
                    else
                    {
                        ApplicationMetricDescription.Validate(metric);
                    }
                }
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_UPDATE_DESCRIPTION();

            var flags = NativeTypes.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_NONE;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.RemoveApplicationCapacity = NativeTypes.ToBOOLEAN(RemoveApplicationCapacity);

            if (this.MinimumNodes.HasValue)
            {
                flags = flags | NativeTypes.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MINNODES;
                nativeDescription.MinimumNodes = checked((uint)this.MinimumNodes.Value);
            }

            if (this.MaximumNodes.HasValue)
            {
                flags = flags | NativeTypes.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_MAXNODES;
                nativeDescription.MaximumNodes = checked((uint)this.MaximumNodes.Value);
            }

            if (this.Metrics != null)
            {
                flags = flags | NativeTypes.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS.FABRIC_APPLICATION_UPDATE_DESCRIPTION_FLAGS_METRICS;
                var nativeMetricsList = new NativeTypes.FABRIC_APPLICATION_METRIC_LIST();
                nativeMetricsList.Count = (uint)this.Metrics.Count;
                if (this.Metrics.Count > 0)
                {
                    var nativeMetricsArray = new NativeTypes.FABRIC_APPLICATION_METRIC_DESCRIPTION[this.Metrics.Count];
                    for (int i = 0; i < this.Metrics.Count; i++)
                    {
                        Metrics[i].ToNative(pinCollection, ref nativeMetricsArray[i]);
                    }

                    nativeMetricsList.Metrics = pinCollection.AddBlittable(nativeMetricsArray);
                }
                nativeDescription.Metrics = pinCollection.AddBlittable(nativeMetricsList);
            }

            nativeDescription.Flags = (uint)flags;

            return pinCollection.AddBlittable(nativeDescription);
        }

    }
}