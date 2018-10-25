// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents a health evaluation which describes 
    /// the data and the algorithm used by health store to evaluate the health of an entity.</para>
    /// </summary>
    public abstract class HealthEvaluation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthEvaluation" /> class.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>The <see cref="System.Fabric.Health.HealthEvaluationKind" /> of the evaluation.</para>
        /// </param>
        protected HealthEvaluation(HealthEvaluationKind kind)
        {
            this.Kind = kind;
        }

        // Default order is -1. A lower value like -2 will cause this property to appear before others.
        /// <summary>
        /// <para>Gets the kind of the health evaluation.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.Health.HealthEvaluationKind" /> of the health evaluation.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public HealthEvaluationKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the description of the health evaluation, 
        /// which represents a summary of the evaluation process.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> representing the description of the health evaluation.</para>
        /// </value>
        public string Description
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>The aggregated health state of the evaluated entity.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.Health.HealthState" /> representing the aggregated health state.</para>
        /// </value>
        public HealthState AggregatedHealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the health evaluation.
        /// </summary>
        /// <returns>A string representation of the health evaluation.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "HealthState: {0}: {1}", this.AggregatedHealthState, this.Description);
        }

        internal static unsafe HealthEvaluation FromNativeValue(NativeTypes.FABRIC_HEALTH_EVALUATION nativeHealthEvaluation)
        {
            switch (nativeHealthEvaluation.Kind)
            {
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_EVENT:
                    return EventHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
                    return PartitionsHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
                    return ReplicasHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
                    return DeployedServicePackagesHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
                    return DeployedApplicationsHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
                    return ServicesHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_NODES:
                    return NodesHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS:
                    return ApplicationsHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES:
                    return UpgradeDomainNodesHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
                    return UpgradeDomainDeployedApplicationsHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION:
                    return SystemApplicationHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_PARTITION:
                    return PartitionHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_REPLICA:
                    return ReplicaHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE:
                    return DeployedServicePackageHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION:
                    return DeployedApplicationHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SERVICE:
                    return ServiceHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_NODE:
                    return NodeHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATION:
                    return ApplicationHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK:
                    return DeltaNodesCheckHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK:
                    return UpgradeDomainDeltaNodesCheckHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                case NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS:
                    return ApplicationTypeApplicationsHealthEvaluation.FromNative(nativeHealthEvaluation.Value);
                default:
                    // The kind may be invalid because it was send by a newer version of the server.
                    // Return null. It's the user's responsibility to check for null.
                    // Another option is to throw an exception, such as:
                    // throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeEnumInvalid_Formatted, nativeHealthEvaluation.Kind));
                    // Since this method is only intended for internal use, return null as an optimization.
                    AppTrace.TraceSource.WriteError("HealthEvaluationKind.FromNative", "Unknown FABRIC_HEALTH_EVALUATION_KIND: {0}", nativeHealthEvaluation.Kind);
                    return null;

            }
        }

        internal static unsafe List<HealthEvaluation> FromNativeList(IntPtr nativeHealthEvaluationPtr)
        {
            var unhealthyEvaluations = new List<HealthEvaluation>();
            if (nativeHealthEvaluationPtr != IntPtr.Zero)
            {
                var nativeEvaluations = (NativeTypes.FABRIC_HEALTH_EVALUATION_LIST*)nativeHealthEvaluationPtr;
                for (int i = 0; i < nativeEvaluations->Count; i++)
                {
                    var nativeEvaluation = (NativeTypes.FABRIC_HEALTH_EVALUATION*)((ulong)nativeEvaluations->Items + (ulong)(i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_HEALTH_EVALUATION))));
                    var managedEvaluation = HealthEvaluation.FromNativeValue(*nativeEvaluation);
                    if (managedEvaluation != null)
                    {
                        unhealthyEvaluations.Add(managedEvaluation);
                    }
                }
            }

            return unhealthyEvaluations;
        }

        internal static Type GetDerivedHealthEvaluationClassTypeFromKind(HealthEvaluationKind kind)
        {
            var targetTypeName = string.Format(CultureInfo.InvariantCulture, "{0}.{1}HealthEvaluation", typeof(HealthEvaluationKind).Namespace, kind);
            var targetType = Type.GetType(targetTypeName);
            if (targetType == null)
            {
                AppTrace.TraceSource.WriteError("HealthEvaluation.GetDerivedHealthEvaluationClassTypeFromKind():", "Could not find derived class related to unknown FABRIC_HEALTH_EVALUATION_KIND: {0}", kind);
                ReleaseAssert.Failfast(
                    string.Format(
                        CultureInfo.CurrentCulture, 
                        StringResources.Error_EnumInvalid_Formatted,
                        kind,
                        "HealthEvaluationKind"));
            }

            return targetType;
        }
    }
}