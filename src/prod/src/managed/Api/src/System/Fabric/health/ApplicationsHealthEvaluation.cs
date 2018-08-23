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
    /// <para>Represents health evaluation for applications, containing health evaluations for each unhealthy application that impacted current 
    /// aggregated health state.</para>
    /// </summary>
    /// <remarks>The applications evaluation can be returned during cluster health evaluation, when the aggregated health state 
    /// of the cluster is either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</remarks>
    public sealed class ApplicationsHealthEvaluation : HealthEvaluation
    {
        internal ApplicationsHealthEvaluation()
            : base(HealthEvaluationKind.Applications)
        {
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>A list of <see cref="System.Fabric.Health.HealthEvaluation" /> representing 
        /// the unhealthy evaluations that led to current aggregated health state.</para>
        /// </value>
        /// <remarks>
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.ApplicationHealthEvaluation" /> that impacted the aggregated health.</remarks>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of unhealthy applications from the <see cref="System.Fabric.Health.ClusterHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy applications.</para>
        /// </value>
        public byte MaxPercentUnhealthyApplications
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of applications in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>An <see cref="System.Int64" /> representing the total count of applications in the health store.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe ApplicationsHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_APPLICATIONS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ApplicationsHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyApplications = nativeHealthEvaluation.MaxPercentUnhealthyApplications;

            return managedHealthEvaluation;
        }
    }
}