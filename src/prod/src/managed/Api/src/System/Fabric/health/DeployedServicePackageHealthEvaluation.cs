// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents health evaluation for a deployed service package,
    /// containing information about the data and the algorithm used by 
    /// health store to evaluate health.
    /// The evaluation is returned only when the aggregated health state is either 
    /// <see cref="System.Fabric.Health.HealthState.Error" /> or <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
    /// </summary>
    public sealed class DeployedServicePackageHealthEvaluation : HealthEvaluation
    {
        internal DeployedServicePackageHealthEvaluation()
            : base(HealthEvaluationKind.DeployedServicePackage)
        {
            this.ServicePackageActivationId = string.Empty;
        }

        /// <summary>
        /// <para>Gets the name of the application.</para>
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
        /// <para>Gets the name of the node where the deployed service package is running.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        public string ServiceManifestName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the ActivationId of deployed service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing ActivationId of deployed service package. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Health.DeployedServicePackageHealthEvaluation.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the unhealthy evaluations that led to the current aggregated health state. 
        /// The type of the unhealthy evaluations can 
        /// be <see cref="System.Fabric.Health.EventHealthEvaluation" />.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that led to current aggregated health state.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations
        {
            get;
            internal set;
        }

        internal static unsafe DeployedServicePackageHealthEvaluation FromNative(IntPtr nativeHealthEvaluationValuePtr)
        {
            ReleaseAssert.AssertIf(nativeHealthEvaluationValuePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "nativeHealthEvaluationValue"));
            var nativeHealthEvaluation = *(NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION*)nativeHealthEvaluationValuePtr;

            var managedHealthEvaluation = new DeployedServicePackageHealthEvaluation();

            managedHealthEvaluation.Description = NativeTypes.FromNativeString(nativeHealthEvaluation.Description);
            managedHealthEvaluation.AggregatedHealthState = (HealthState)nativeHealthEvaluation.AggregatedHealthState;
            managedHealthEvaluation.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEvaluation.UnhealthyEvaluations);
            managedHealthEvaluation.ApplicationName = NativeTypes.FromNativeUri(nativeHealthEvaluation.ApplicationName);
            managedHealthEvaluation.NodeName = NativeTypes.FromNativeString(nativeHealthEvaluation.NodeName);
            managedHealthEvaluation.ServiceManifestName = NativeTypes.FromNativeString(nativeHealthEvaluation.ServiceManifestName);

            if (nativeHealthEvaluation.Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION_EX1*)nativeHealthEvaluation.Reserved;

                managedHealthEvaluation.ServicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx1->ServicePackageActivationId);
            }

            return managedHealthEvaluation;
        }
    }
}