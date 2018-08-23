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
    /// <para>Represents health evaluation for services of a certain service type belonging to an application,
    /// containing health evaluations for 
    /// each unhealthy service that impacted current aggregated health state.
    /// Can be returned when evaluating application health and the aggregated 
    /// health state is either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class ServicesHealthEvaluation : HealthEvaluation
    {
        internal ServicesHealthEvaluation()
            : base(HealthEvaluationKind.Services)
        {
        }

        /// <summary>
        /// <para>Gets the service type name.</para>
        /// </summary>
        /// <value>
        /// <para>The service type name.</para>
        /// </value>
        public string ServiceTypeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state.
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.ServiceHealthEvaluation" /> that impacted the aggregated health.</para>
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
        /// unhealthy services from the <see cref="System.Fabric.Health.ServiceTypeHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy services.</para>
        /// </value>
        public byte MaxPercentUnhealthyServices
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of services of the current service type in the application from the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of services of the current service type in the specified application.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe ServicesHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_SERVICES_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ServicesHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.ServiceTypeName = NativeTypes.FromNativeString(nativeHealthEvaluation.ServiceTypeName);
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyServices = nativeHealthEvaluation.MaxPercentUnhealthyServices;

            return managedHealthEvaluation;
        }
    }
}