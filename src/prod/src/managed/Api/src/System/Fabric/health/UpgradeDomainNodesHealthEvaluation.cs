// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents health evaluation for cluster nodes in an upgrade domain,
    /// containing health evaluations for each unhealthy node that 
    /// impacted current aggregated health state.
    /// Can be returned when evaluating cluster health during cluster upgrade and the aggregated health 
    /// state is either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class UpgradeDomainNodesHealthEvaluation : HealthEvaluation
    {
        internal UpgradeDomainNodesHealthEvaluation()
            : base(HealthEvaluationKind.UpgradeDomainNodes)
        {
        }

        /// <summary>
        /// <para>Gets the name of the upgrade domain where nodes health is currently evaluated.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade domain name.</para>
        /// </value>
        public string UpgradeDomainName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state.
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.NodeHealthEvaluation" /> that impacted the aggregated health.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of unhealthy nodes from the <see cref="System.Fabric.Health.ClusterHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy nodes.</para>
        /// </value>
        public byte MaxPercentUnhealthyNodes
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of nodes in the current upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of nodes in the current upgrade domain.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe UpgradeDomainNodesHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_UPGRADE_DOMAIN_NODES_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new UpgradeDomainNodesHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UpgradeDomainName = NativeTypes.FromNativeString(nativeHealthEvaluation.UpgradeDomainName);
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyNodes = nativeHealthEvaluation.MaxPercentUnhealthyNodes;

            return managedHealthEvaluation;
        }
    }
}