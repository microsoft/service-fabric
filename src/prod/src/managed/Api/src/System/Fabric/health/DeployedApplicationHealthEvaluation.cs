// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents health evaluation for a deployed application,
    /// containing information about the data and the algorithm used by health store 
    /// to evaluate health. </para>
    /// </summary>
    /// <remarks>The evaluation is returned only when the aggregated health state is either <see cref="System.Fabric.Health.HealthState.Error" /> 
    /// or <see cref="System.Fabric.Health.HealthState.Warning" />.</remarks>
    public sealed class DeployedApplicationHealthEvaluation : HealthEvaluation
    {
        internal DeployedApplicationHealthEvaluation()
            : base(HealthEvaluationKind.DeployedApplication)
        {
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the application name.</para>
        /// </value>
        [JsonCustomization(PropertyName = "Name")]
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the name of the node where the deployed application is running.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that led to the current aggregated health state 
        /// of the deployed application. The types of the 
        /// unhealthy evaluations can be <see cref="System.Fabric.Health.DeployedServicePackagesHealthEvaluation" /> or 
        /// <see cref="System.Fabric.Health.EventHealthEvaluation" />.</para>
        /// </summary>
        /// <value>
        /// <para>The list of unhealthy evaluations that led to the current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe DeployedApplicationHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new DeployedApplicationHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.ApplicationName = NativeTypes.FromNativeUri(nativeHealthEvaluation.ApplicationName);
            managedHealthEvaluation.NodeName = NativeTypes.FromNativeString(nativeHealthEvaluation.NodeName);

            return managedHealthEvaluation;
        }
    }
}