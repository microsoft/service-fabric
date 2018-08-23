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
    /// <para>Represents health evaluation for delta nodes, 
    /// containing health evaluations for each unhealthy node that impacted current 
    /// aggregated health state. 
    /// Can be returned during cluster upgrade when the aggregated health state of the cluster is 
    /// <see cref="System.Fabric.Health.HealthState.Error" />.</para>
    /// </summary>
    public sealed class DeltaNodesCheckHealthEvaluation : HealthEvaluation
    {
        internal DeltaNodesCheckHealthEvaluation()
            : base(HealthEvaluationKind.DeltaNodesCheck)
        {
        }

        /// <summary>
        /// <para>Gets the number of nodes with aggregated heath state <see cref="System.Fabric.Health.HealthState.Error" /> in the health store 
        /// at the beginning of the cluster upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The number of nodes with aggregated heath state <see cref="System.Fabric.Health.HealthState.Error" /> in the health store at 
        /// the beginning of the cluster upgrade.</para>
        /// </value>
        public long BaselineErrorCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of nodes in the health store at the beginning of the cluster upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of nodes in the health store at the beginning of the cluster upgrade.</para>
        /// </value>
        public long BaselineTotalCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the current total number of nodes in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The current total number of nodes in the health store.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of delta unhealthy nodes from the <see cref="System.Fabric.Health.ClusterUpgradeHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of delta unhealthy nodes.</para>
        /// </value>
        public byte MaxPercentDeltaUnhealthyNodes
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
        /// <para>Returns a list of the unhealthy evaluations that 
        /// led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe DeltaNodesCheckHealthEvaluation FromNative(IntPtr nativeDeltaNodesCheckHealthEvaluationPtr)
        {
            ReleaseAssert.AssertIf(nativeDeltaNodesCheckHealthEvaluationPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "DeltaNodesCheckHealthEvaluation"));
            var nativeDeltaNodesCheckHealthEvaluation = *(NativeTypes.FABRIC_DELTA_NODES_CHECK_HEALTH_EVALUATION*)nativeDeltaNodesCheckHealthEvaluationPtr;

            var managedDeltaNodesCheckHealthEvaluation = new DeltaNodesCheckHealthEvaluation();
            managedDeltaNodesCheckHealthEvaluation.Description = NativeTypes.FromNativeString(nativeDeltaNodesCheckHealthEvaluation.Description);
            managedDeltaNodesCheckHealthEvaluation.AggregatedHealthState = (HealthState)nativeDeltaNodesCheckHealthEvaluation.AggregatedHealthState;
            managedDeltaNodesCheckHealthEvaluation.BaselineErrorCount = (long)nativeDeltaNodesCheckHealthEvaluation.BaselineErrorCount;
            managedDeltaNodesCheckHealthEvaluation.BaselineTotalCount = (long)nativeDeltaNodesCheckHealthEvaluation.BaselineTotalCount;
            managedDeltaNodesCheckHealthEvaluation.TotalCount = (long)nativeDeltaNodesCheckHealthEvaluation.TotalCount;
            managedDeltaNodesCheckHealthEvaluation.MaxPercentDeltaUnhealthyNodes = nativeDeltaNodesCheckHealthEvaluation.MaxPercentDeltaUnhealthyNodes;
            managedDeltaNodesCheckHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeDeltaNodesCheckHealthEvaluation.UnhealthyEvaluations);

            return managedDeltaNodesCheckHealthEvaluation;
        }
    }
}
