// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Defines a health policy used to evaluate the health of the cluster or of a cluster node.</para>
    /// </summary>
    public class ClusterHealthPolicy
    {
        internal static readonly ClusterHealthPolicy Default = new ClusterHealthPolicy();

        private bool considerWarningAsError;
        private byte maxPercentUnhealthyNodes;
        private byte maxPercentUnhealthyApplications;
        private ApplicationTypeHealthPolicyMap applicationTypeHealthPolicyMap;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ClusterHealthPolicy" /> class.</para>
        /// </summary>
        /// <remarks>By default, no errors or warnings are tolerated.</remarks>
        public ClusterHealthPolicy()
        {
            this.considerWarningAsError = false;
            this.maxPercentUnhealthyNodes = 0;
            this.maxPercentUnhealthyApplications = 0;
            this.applicationTypeHealthPolicyMap = new ApplicationTypeHealthPolicyMap();
        }

        /// <summary>
        /// <para>Gets or sets a <see cref="System.Boolean" /> that determines whether�reports with warning state should be treated with the same severity as errors.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if reports with warning state should be treated as errors; <languageKeyword>false</languageKeyword> if warnings 
        /// should not be treated as errors.</para>
        /// </value>
        public bool ConsiderWarningAsError
        {
            get
            {
                return this.considerWarningAsError;
            }

            set
            {
                this.considerWarningAsError = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy nodes.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy nodes. Allowed values are integer values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of nodes that can be unhealthy 
        /// before the cluster is considered in error. If the percentage is respected but there is at least one unhealthy node,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy nodes
        /// over the total number of nodes in the cluster.
        /// The computation rounds up to tolerate one failure on small numbers of nodes. Default percentage: zero.
        /// </para>
        /// <para>In large clusters, some nodes will always be down or out for repairs, so this percentage should be configured to tolerate that.</para>
        /// </remarks>
        public byte MaxPercentUnhealthyNodes
        {
            get
            {
                return this.maxPercentUnhealthyNodes;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyNodes = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy applications.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy applications. Allowed values are integer values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of applications that can be unhealthy 
        /// before the cluster is considered in error. If the percentage is respected but there is at least one unhealthy application,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy applications
        /// over the total number of applications deployed in the cluster, excluding all applications of application types
        /// that are included in the <see cref="System.Fabric.Health.ApplicationTypeHealthPolicyMap"/>.
        /// The computation rounds up to tolerate one failure on small numbers of applications. Default percentage: zero.
        /// </para>
        /// </remarks>
        public byte MaxPercentUnhealthyApplications
        {
            get
            {
                return this.maxPercentUnhealthyApplications;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyApplications = value;
            }
        }

        /// <summary>
        /// Gets a string representation of the cluster health policy.
        /// </summary>
        /// <returns>A string representation of the cluster health policy.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "ClusterHealthPolicy: MaxPercentUnhealthyNodes={0}, MaxPercentUnhealthyApplications={1}, ConsiderWarningAsError={2}",
                this.MaxPercentUnhealthyNodes,
                this.MaxPercentUnhealthyApplications,
                this.ConsiderWarningAsError);
        }

        /// <summary>
        /// <para>
        /// Gets the map with MaxPercentUnhealthyApplications per application type name. 
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The application type health policy map with MaxPercentUnhealthyApplications per application type name.</para>
        /// </value>
        /// <remarks>
        /// <para>The application type health policy map can be used during cluster health evaluation to describe special application types. 
        /// By default, all applications are put into a pool and evaluated with <see cref="System.Fabric.Health.ClusterHealthPolicy.MaxPercentUnhealthyApplications"/>. If one or more application types are special 
        /// and should be treated in a different way, they can be taken out of the global pool and evaluated against the percentages associated with their application type name in the map. 
        /// For example, in a cluster there are thousands of applications of different types, and a few control application instances of a special application type.
        /// The control applications should never be in error. So users can specify global MaxPercentUnhealthyApplications to 20% to tolerate some failures, 
        /// but for the application type "ControlApplicationType" set the MaxPercentUnhealthyApplications to 0. 
        /// This way, if some of the many applications are unhealthy, but below the global unhealthy percentage, the cluster would be evaluated to Warning. 
        /// A warning health state does not impact cluster upgrade or other monitoring triggered by Error health state.
        /// But even one control application in error would make cluster health error, which can rollback or prevent a cluster upgrade. </para>>
        /// <para>For the application types defined in the map, all application instances are taken out of the global pool of applications.
        /// They are evaluated based on the total number of applications of the application type, using the specific MaxPercentUnhealthyApplications from the map.
        /// All the rest of the applications remain in the global pool and are evaluated with MaxPercentUnhealthyApplications.</para>
        /// <para>To define entries for the specific application types in the cluster manifest, inside FabricSettings add entries
        /// for parameters with name formed by prefix "ApplicationTypeMaxPercentUnhealthyApplications-" followed by application type name.</para>
        /// <para>If no policy is specified for an application type, the default MaxPercentUnhealthyApplications is used for evaluation.</para>
        /// <para>The application type health evaluation is done only when the cluster is configured with EnableApplicationTypeHealthEvaluation <languageKeyword>true</languageKeyword>.
        /// The setting is disabled by default.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsIgnored = true)]
        public ApplicationTypeHealthPolicyMap ApplicationTypeHealthPolicyMap
        {
            get
            {
                return this.applicationTypeHealthPolicyMap;
            }
        }

        internal static bool AreEqual(ClusterHealthPolicy current, ClusterHealthPolicy other)
        {
            if ((current != null) && (other != null))
            {
                bool equals = (current.ConsiderWarningAsError == other.ConsiderWarningAsError) &&
                    (current.MaxPercentUnhealthyNodes == other.MaxPercentUnhealthyNodes) &&
                    (current.MaxPercentUnhealthyApplications == other.MaxPercentUnhealthyApplications);
                if (!equals) { return false; }

                equals = ApplicationTypeHealthPolicyMap.AreEqual(current.ApplicationTypeHealthPolicyMap, other.ApplicationTypeHealthPolicyMap);
                if (!equals) { return false; }

                return true;
            }
            else
            {
                return (current == null) && (other == null);
            }
        }

        internal static unsafe ClusterHealthPolicy FromNative(IntPtr nativeClusterHealthPolicyPtr)
        {
            var clusterHealthPolicy = new ClusterHealthPolicy();
            var nativeClusterHealthPolicy = (NativeTypes.FABRIC_CLUSTER_HEALTH_POLICY*)nativeClusterHealthPolicyPtr;

            clusterHealthPolicy.ConsiderWarningAsError = NativeTypes.FromBOOLEAN(nativeClusterHealthPolicy->ConsiderWarningAsError);
            clusterHealthPolicy.MaxPercentUnhealthyNodes = nativeClusterHealthPolicy->MaxPercentUnhealthyNodes;
            clusterHealthPolicy.MaxPercentUnhealthyApplications = nativeClusterHealthPolicy->MaxPercentUnhealthyApplications;

            if (nativeClusterHealthPolicy->Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_CLUSTER_HEALTH_POLICY_EX1*)nativeClusterHealthPolicy->Reserved;
                clusterHealthPolicy.ApplicationTypeHealthPolicyMap.FromNative(ex1->ApplicationTypeHealthPolicyMap);
            }

            return clusterHealthPolicy;
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeClusterHealthPolicy = new NativeTypes.FABRIC_CLUSTER_HEALTH_POLICY();

            nativeClusterHealthPolicy.ConsiderWarningAsError = NativeTypes.ToBOOLEAN(this.ConsiderWarningAsError);
            nativeClusterHealthPolicy.MaxPercentUnhealthyNodes = this.MaxPercentUnhealthyNodes;
            nativeClusterHealthPolicy.MaxPercentUnhealthyApplications = this.MaxPercentUnhealthyApplications;

            var ex1 = new NativeTypes.FABRIC_CLUSTER_HEALTH_POLICY_EX1();
            ex1.ApplicationTypeHealthPolicyMap = this.ApplicationTypeHealthPolicyMap.ToNative(pin);
            ex1.Reserved = IntPtr.Zero;
            nativeClusterHealthPolicy.Reserved = pin.AddBlittable(ex1);

            return pin.AddBlittable(nativeClusterHealthPolicy);
        }
    }
}