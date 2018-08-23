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
    /// <para>Represents health evaluation for a replica, 
    /// containing information about the data and the algorithm used by health store to 
    /// evaluate health. The evaluation is returned only when the aggregated health state is either <see cref="System.Fabric.Health.HealthState.Error" /> 
    /// or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class ReplicaHealthEvaluation : HealthEvaluation
    {
        internal ReplicaHealthEvaluation()
            : base(HealthEvaluationKind.Replica)
        {
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that led to the current aggregated health state of the replica. The types of the unhealthy 
        /// evaluations can be <see cref="System.Fabric.Health.EventHealthEvaluation" />.</para>
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
        /// <para>Gets the stateful service replica ID or the stateless service instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica or instance ID.</para>
        /// </value>
        public long ReplicaOrInstanceId
        {
            get;
            internal set;
        }

        internal static unsafe ReplicaHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_REPLICA_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ReplicaHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.PartitionId = nativeHealthEvaluation.PartitionId;
            managedHealthEvaluation.ReplicaOrInstanceId = nativeHealthEvaluation.ReplicaOrInstanceId;

            return managedHealthEvaluation;
        }
    }
}