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
    /// <para>Represents health evaluation for deployed applications, 
    /// containing health evaluations for each unhealthy deployed application that impacted 
    /// current aggregated health state. 
    /// Can be returned when evaluating application health and the aggregated health state is 
    /// either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class DeployedApplicationsHealthEvaluation : HealthEvaluation
    {
        internal DeployedApplicationsHealthEvaluation()
            : base(HealthEvaluationKind.DeployedApplications)
        {
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state. 
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.DeployedApplicationHealthEvaluation" /> that impacted the aggregated health.</para>
        /// </summary>
        /// <value>
        /// <para>A list of unhealthy 
        /// evaluations that led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of unhealthy deployed applications
        /// from the <see cref="System.Fabric.Health.ApplicationHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Byte" /> representing the maximum allowed percentage 
        /// of unhealthy deployed applications.</para>
        /// </value>
        public byte MaxPercentUnhealthyDeployedApplications
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of deployed applications of the application in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>An <see cref="System.Int64" /> representing the total count of 
        /// deployed applications of the application in the health store.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe DeployedApplicationsHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new DeployedApplicationsHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyDeployedApplications = nativeHealthEvaluation.MaxPercentUnhealthyDeployedApplications;

            return managedHealthEvaluation;
        }
    }
}