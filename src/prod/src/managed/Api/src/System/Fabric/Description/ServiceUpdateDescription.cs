// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes changes to the <see cref="System.Fabric.Description.ServiceDescription" /> of an existing service.</para>
    /// </summary>
    public abstract class ServiceUpdateDescription
    {
        /// <summary>
        /// <para>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.ServiceUpdateDescription" /> class with no changes specified.
        /// The relevant properties must be explicitly set to specify changes.
        /// </para>
        /// </summary>
        /// <param name="kind">
        /// <para>
        /// The kind specifies the derived type of this instance
        /// (e.g. <see cref="System.Fabric.Description.StatelessServiceUpdateDescription" /> or <see cref="System.Fabric.Description.StatefulServiceUpdateDescription" />).
        /// </para>
        /// </param>
        protected ServiceUpdateDescription(ServiceDescriptionKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>
        /// Gets a value indicating the derived type of this instance 
        /// (e.g. <see cref="System.Fabric.Description.StatelessServiceUpdateDescription" /> or <see cref="System.Fabric.Description.StatefulServiceUpdateDescription" />).
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The derived type of this instance.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.Kind" /></para>
        /// </remarks>
        public ServiceDescriptionKind Kind { get; private set; }

        /// <summary>
        /// <para>
        /// Gets or sets the placement constraints for this service, which restricts the nodes where replicas of this service can be placed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The placement constraints expression.</para>
        /// </value>
        /// <example> nodeName == node1 || nodeType == databaseNode </example>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.PlacementConstraints" /></para>
        /// </remarks>
        public string PlacementConstraints { get; set; }

        /// <summary>
        /// <para>Gets or sets a map of service load metric names to service load metric descriptions.</para>
        /// </summary>
        /// <value>
        /// <para>The map of service load metrics.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.Metrics" /></para>
        /// </remarks>
        public KeyedCollection<string, ServiceLoadMetricDescription> Metrics { get; set; }

        /// <summary>
        /// <para>Gets or sets a list of service correlations.</para>
        /// </summary>
        /// <value>
        /// <para>The list of service correlations.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.Correlations" /></para>
        /// </remarks>
        public IList<ServiceCorrelationDescription> Correlations { get; set; }

        /// <summary>
        /// <para>Gets or sets a list of service placement policies.</para>
        /// </summary>
        /// <value>
        /// <para>The list of service placement policies.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.PlacementPolicies" /></para>
        /// </remarks>
        public IList<ServicePlacementPolicyDescription> PlacementPolicies { get; set; }

        /// <summary>
        /// <para>Gets or sets a description of service partitioning changes.</para>
        /// </summary>
        /// <value>
        /// <para>The service partitioning changes.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.PartitionSchemeDescription" /></para>
        /// </remarks>
        public RepartitionDescription RepartitionDescription { get; set; }

        /// <summary>
        /// <para>Gets or sets the default move cost.</para>
        /// </summary>
        /// <value>
        /// <para>The default move cost.</para>
        /// </value>
        /// <remarks>
        /// <para>See <see cref="System.Fabric.Description.ServiceDescription.DefaultMoveCost" /></para>
        /// </remarks>
        public MoveCost? DefaultMoveCost
        {
            get;
            set;
        }

        /// <summary>
        /// <para>
        /// Gets or sets the list <see cref="System.Fabric.Description.ScalingPolicyDescription" /> for this service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The list of scaling policies associated with this service. </para>
        /// </value>
        public IList<ScalingPolicyDescription> ScalingPolicies { get; set; }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeUpdateDescription = new NativeTypes.FABRIC_SERVICE_UPDATE_DESCRIPTION[1];

            NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind;
            nativeUpdateDescription[0].Value = this.ToNative(pin, out kind);
            nativeUpdateDescription[0].Kind = kind;

            return pin.AddBlittable(nativeUpdateDescription);
        }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind);
        internal Tuple<uint, IntPtr> ToNativeCorrelations(PinCollection pin)
        {
            if (this.Correlations.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var correlationArray = new NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION[this.Correlations.Count];
            for (int i = 0; i < this.Correlations.Count; i++)
            {
                this.Correlations[i].ToNative(pin, ref correlationArray[i]);
            }

            return Tuple.Create((uint)this.Correlations.Count, pin.AddBlittable(correlationArray));
        }

        internal Tuple<uint, IntPtr> ToNativePolicies(PinCollection pin)
        {
            if (this.PlacementPolicies.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var policyArray = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION[this.PlacementPolicies.Count];
            for (int i = 0; i < this.PlacementPolicies.Count; i++)
            {
                this.PlacementPolicies[i].ToNative(pin, ref policyArray[i]);
            }

            return Tuple.Create((uint)this.PlacementPolicies.Count, pin.AddBlittable(policyArray));
        }

        internal Tuple<uint, IntPtr> ToNativeScalingPolicies(PinCollection pin)
        {
            if (this.ScalingPolicies == null || this.ScalingPolicies.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var scalingArray = new NativeTypes.FABRIC_SCALING_POLICY[this.ScalingPolicies.Count];
            for (int i = 0; i < this.ScalingPolicies.Count; i++)
            {
                this.ScalingPolicies[i].ToNative(pin, ref scalingArray[i]);
            }

            return Tuple.Create((uint)this.ScalingPolicies.Count, pin.AddBlittable(scalingArray));
        }

        internal unsafe void ParseLoadMetrics(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            bool isStateful = (this.Kind == ServiceDescriptionKind.Stateful);

            var nativeMetrics = (NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServiceLoadMetricDescription.CreateFromNative((IntPtr)(nativeMetrics + i), isStateful);
                this.Metrics.Add(item);
            }
        }

        internal unsafe void ParseCorrelations(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            var nativeCorrelations = (NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServiceCorrelationDescription.CreateFromNative((IntPtr)(nativeCorrelations + i));
                this.Correlations.Add(item);
            }
        }

        internal unsafe void ParsePlacementPolicies(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            var nativePolicies = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServicePlacementPolicyDescription.CreateFromNative((IntPtr)(nativePolicies + i));
                this.PlacementPolicies.Add(item);
            }
        }

        internal unsafe void ParseScalingPolicies(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }
            var nativeScaling = (NativeTypes.FABRIC_SCALING_POLICY*)array;

            for (int i = 0; i < count; ++i)
            {
                var item = ScalingPolicyDescription.CreateFromNative((IntPtr)(nativeScaling + i));
                this.ScalingPolicies.Add(item);
            }
        }
    }
}