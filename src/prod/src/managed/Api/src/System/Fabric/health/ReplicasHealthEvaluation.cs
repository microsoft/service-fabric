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
    /// <para>Represents health evaluation for replicas,
    /// containing health evaluations for each unhealthy replica that impacted current 
    /// aggregated health state.
    /// Can be returned when evaluating partition health and the aggregated health state is either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class ReplicasHealthEvaluation : HealthEvaluation
    {
        internal ReplicasHealthEvaluation()
            : base(HealthEvaluationKind.Replicas)
        {
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state.
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.ReplicaHealthEvaluation" /> that impacted the aggregated health.</para>
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
        /// <para>Gets the maximum allowed percentage of 
        /// unhealthy replicas per partition from the <see cref="System.Fabric.Health.ApplicationHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy replicas per partition.</para>
        /// </value>
        public byte MaxPercentUnhealthyReplicasPerPartition
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of replicas in the partition from the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of replicas in the partition.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe ReplicasHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_REPLICAS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ReplicasHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyReplicasPerPartition = nativeHealthEvaluation.MaxPercentUnhealthyReplicasPerPartition;

            return managedHealthEvaluation;
        }
    }
}