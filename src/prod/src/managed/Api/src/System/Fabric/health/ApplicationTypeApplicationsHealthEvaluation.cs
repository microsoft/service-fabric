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
    /// <para>
    /// Represents health evaluation for applications of an application type.
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>The application type applications evaluation can be returned when 
    /// cluster health evaluation returns unhealthy aggregated health state, 
    /// either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.
    /// It contains health evaluations for each unhealthy application 
    /// of the included application type that impacted current aggregated health state.</para>
    /// </remarks>
    public sealed class ApplicationTypeApplicationsHealthEvaluation : HealthEvaluation
    {
        internal ApplicationTypeApplicationsHealthEvaluation()
            : base(HealthEvaluationKind.ApplicationTypeApplications)
        {
        }

        /// <summary>
        /// <para>
        /// Gets the application type name.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        public string ApplicationTypeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>
        /// Gets the list of unhealthy evaluations that led to the aggregated health state.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The list of unhealthy evaluations for the specified application type.</para>
        /// </value>
        /// <remarks> Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.ApplicationHealthEvaluation" /> of this application type that impacted the aggregated health.</remarks>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>
        /// Gets the maximum allowed percentage of unhealthy applications for the application type, specified as an entry in 
        /// <see cref="System.Fabric.Health.ApplicationTypeHealthPolicyMap" />.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy applications for the application type.</para>
        /// </value>
        public byte MaxPercentUnhealthyApplications
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>
        /// Gets the total number of applications of the application type found in the health store.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The total number of applications of the application type.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe ApplicationTypeApplicationsHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_APPLICATION_TYPE_APPLICATIONS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ApplicationTypeApplicationsHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.ApplicationTypeName = NativeTypes.FromNativeString(nativeHealthEvaluation.ApplicationTypeName);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyApplications = nativeHealthEvaluation.MaxPercentUnhealthyApplications;

            return managedHealthEvaluation;
        }
    }
}