// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents health evaluation for delta unhealthy cluster nodes in an upgrade domain, containing health evaluations for each 
    /// unhealthy node that impacted current aggregated health state.
    /// Can be returned during cluster upgrade when cluster aggregated health 
    /// state is <see cref="System.Fabric.Health.HealthState.Error" />.</para>
    /// </summary>
    public sealed class UpgradeDomainDeltaNodesCheckHealthEvaluation : HealthEvaluation
    {
        internal UpgradeDomainDeltaNodesCheckHealthEvaluation()
            : base(HealthEvaluationKind.UpgradeDomainDeltaNodesCheck)
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
        /// <para>Gets the number of upgrade domain nodes with aggregated heath state 
        /// <see cref="System.Fabric.Health.HealthState.Error" /> in 
        /// the health store at the beginning of the cluster upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The number of upgrade domain nodes with aggregated heath state 
        /// <see cref="System.Fabric.Health.HealthState.Error" /> in the 
        /// health store at the beginning of the cluster upgrade.</para>
        /// </value>
        public long BaselineErrorCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of upgrade domain nodes in the health store
        /// at the beginning of the cluster upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>T he total number of upgrade domain nodes in the health store
        /// at the beginning of the cluster upgrade.</para>
        /// </value>
        public long BaselineTotalCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the current total number of upgrade domain nodes in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The current total number of upgrade domain nodes in the health store.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of upgrade domain delta unhealthy nodes
        /// from the <see cref="System.Fabric.Health.ClusterUpgradeHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of upgrade domain delta unhealthy nodes.</para>
        /// </value>
        public byte MaxPercentUpgradeDomainDeltaUnhealthyNodes
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state. Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.NodeHealthEvaluation" /> that impacted the aggregated health.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that 
        /// led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe UpgradeDomainDeltaNodesCheckHealthEvaluation FromNative(IntPtr nativeUpgradeDomainDeltaNodesCheckHealthEvaluationPtr)
        {
            ReleaseAssert.AssertIf(nativeUpgradeDomainDeltaNodesCheckHealthEvaluationPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "UpgradeDomainDeltaNodesCheckHealthEvaluation"));
            var nativeUpgradeDomainDeltaNodesCheckHealthEvaluation = *(NativeTypes.FABRIC_UPGRADE_DOMAIN_DELTA_NODES_CHECK_HEALTH_EVALUATION*)nativeUpgradeDomainDeltaNodesCheckHealthEvaluationPtr;

            var managedUpgradeDomainDeltaNodesCheckHealthEvaluation = new UpgradeDomainDeltaNodesCheckHealthEvaluation();
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.Description = NativeTypes.FromNativeString(nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.Description);
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.AggregatedHealthState = (HealthState)nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.AggregatedHealthState;
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.UpgradeDomainName = NativeTypes.FromNativeString(nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.UpgradeDomainName);
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.BaselineErrorCount = (long)nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.BaselineErrorCount;
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.BaselineTotalCount = (long)nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.BaselineTotalCount;
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.TotalCount = (long)nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.TotalCount;
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.MaxPercentUpgradeDomainDeltaUnhealthyNodes = nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.MaxPercentUpgradeDomainDeltaUnhealthyNodes;
            managedUpgradeDomainDeltaNodesCheckHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeUpgradeDomainDeltaNodesCheckHealthEvaluation.UnhealthyEvaluations);

            return managedUpgradeDomainDeltaNodesCheckHealthEvaluation;
        }
    }
}
