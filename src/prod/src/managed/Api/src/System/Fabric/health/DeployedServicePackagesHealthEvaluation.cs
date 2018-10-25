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
    /// <para>Represents health evaluation for deployed service packages, 
    /// containing health evaluations for each unhealthy deployed service 
    /// package that impacted current aggregated health state. 
    /// Can be returned when evaluating deployed application health and the aggregated 
    /// health state is either <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class DeployedServicePackagesHealthEvaluation : HealthEvaluation
    {
        internal DeployedServicePackagesHealthEvaluation()
            : base(HealthEvaluationKind.DeployedServicePackages)
        {
        }

        /// <summary>
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state. 
        /// Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.DeployedServicePackageHealthEvaluation" /> that impacted the aggregated health.</para>
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
        /// <para>Gets the total number of deployed service packages of the deployed application in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>An <see cref="System.Int64" /> representing the total count of 
        /// deployed service packages of the deployed 
        /// application in the health store.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe DeployedServicePackagesHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGES_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new DeployedServicePackagesHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;

            return managedHealthEvaluation;
        }
    }
}