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
    /// <para>Represents health evaluation for deployed applications of an application in an upgrade domain,
    /// containing health evaluations for 
    /// each unhealthy deployed application that impacted current aggregated health state.
    /// </para>
    /// </summary>
    /// <remarks>Can be returned when evaluating application health 
    /// during cluster or application upgrade and the aggregated health state is either <see cref="System.Fabric.Health.HealthState.Error" /> 
    /// or <see cref="System.Fabric.Health.HealthState.Warning" />.</remarks>
    public sealed class UpgradeDomainDeployedApplicationsHealthEvaluation : HealthEvaluation
    {
        internal UpgradeDomainDeployedApplicationsHealthEvaluation()
            : base(HealthEvaluationKind.UpgradeDomainDeployedApplications)
        {
        }

        /// <summary>
        /// <para>Gets the name of the upgrade domain where the application health is currently evaluated.</para>
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
        /// <para>Gets the list of unhealthy evaluations that led to the aggregated health state. Includes all the unhealthy 
        /// <see cref="System.Fabric.Health.DeployedApplicationHealthEvaluation" /> that impacted the aggregated health.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations 
        /// that led to current aggregated health state.</para>
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
        /// <para>The maximum allowed percentage of unhealthy deployed applications.</para>
        /// </value>
        public byte MaxPercentUnhealthyDeployedApplications
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the total number of deployed applications of the current application
        /// in the current upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>The total number of deployed applications of the current application
        /// in the current upgrade domain.</para>
        /// </value>
        public long TotalCount
        {
            get;
            internal set;
        }

        internal static unsafe UpgradeDomainDeployedApplicationsHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new UpgradeDomainDeployedApplicationsHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UpgradeDomainName = NativeTypes.FromNativeString(nativeHealthEvaluation.UpgradeDomainName);
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.TotalCount = (long)nativeHealthEvaluation.TotalCount;
            managedHealthEvaluation.MaxPercentUnhealthyDeployedApplications = nativeHealthEvaluation.MaxPercentUnhealthyDeployedApplications;

            return managedHealthEvaluation;
        }
    }
}