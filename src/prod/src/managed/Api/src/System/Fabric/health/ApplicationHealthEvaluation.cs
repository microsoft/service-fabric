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
    /// <para>Represents health evaluation for an application, containing information about the data and the algorithm used by health store to evaluate 
    /// health. </para>
    /// </summary>
    /// <remarks>The evaluation is returned only when the aggregated health state is either <see cref="System.Fabric.Health.HealthState.Error" /> or 
    /// <see cref="System.Fabric.Health.HealthState.Warning" />.</remarks>
    public sealed class ApplicationHealthEvaluation : HealthEvaluation
    {
        internal ApplicationHealthEvaluation()
            : base(HealthEvaluationKind.Application)
        {
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the application name.</para>
        /// </value>
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that led to the current aggregated health state of the application. </para>
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.Health.HealthEvaluation" /> representing the unhealthy 
        /// evaluations that led to current aggregated health state.</para>
        /// </value>
        /// <remarks>The types of the unhealthy evaluations 
        /// can be <see cref="System.Fabric.Health.DeployedApplicationsHealthEvaluation" />, <see cref="System.Fabric.Health.ServicesHealthEvaluation" /> 
        /// or <see cref="System.Fabric.Health.EventHealthEvaluation" />.</remarks>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe ApplicationHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_APPLICATION_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new ApplicationHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.ApplicationName = NativeTypes.FromNativeUri(nativeHealthEvaluation.ApplicationName);

            return managedHealthEvaluation;
        }
    }
}