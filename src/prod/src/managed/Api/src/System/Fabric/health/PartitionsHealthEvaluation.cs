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
    /// <para>Represents health evaluation for the partitions of a service, 
    /// containing health evaluations for each unhealthy partition 
    /// that impacts current aggregated health state. 
    /// Can be returned when evaluating service health and the aggregated health state is 
    /// either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class PartitionsHealthEvaluation : HealthEvaluation
    {
        internal PartitionsHealthEvaluation()
            : base(HealthEvaluationKind.Partitions)
        {
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state. 
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.PartitionHealthEvaluation" /> that impacted the aggregated health.</para>
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
        /// <para>Gets the maximum allowed percentage of unhealthy partitions per service from the <see cref="System.Fabric.Health.ServiceTypeHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy partitions per service.</para>
        /// </value>
        public byte MaxPercentUnhealthyPartitionsPerService
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of partitions of the service from the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of partitions of the service.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe PartitionsHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_PARTITIONS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new PartitionsHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyPartitionsPerService = nativeHealthEvaluation.MaxPercentUnhealthyPartitionsPerService;

            return managedHealthEvaluation;
        }
    }
}