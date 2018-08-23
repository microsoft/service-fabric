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
    /// <para>Represents health evaluation for the fabric:/System application,
    /// containing information about the data and the algorithm used by health 
    /// store to evaluate health. The evaluation is returned only when the aggregated health state of the cluster is either 
    /// <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class SystemApplicationHealthEvaluation : HealthEvaluation
    {
        internal SystemApplicationHealthEvaluation()
            : base(HealthEvaluationKind.SystemApplication)
        {
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that led to the current aggregated health state of the system application. The types of the 
        /// unhealthy evaluations can be <see cref="System.Fabric.Health.DeployedApplicationsHealthEvaluation" />, 
        /// <see cref="System.Fabric.Health.ServicesHealthEvaluation" /> or <see cref="System.Fabric.Health.EventHealthEvaluation" />.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that 
        /// led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe SystemApplicationHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_SYSTEM_APPLICATION_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new SystemApplicationHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);

            return managedHealthEvaluation;
        }
    }
}