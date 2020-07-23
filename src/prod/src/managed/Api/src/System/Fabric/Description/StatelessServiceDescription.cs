// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Extends <see cref="System.Fabric.Description.ServiceDescription" /> to provide additional necessary information to create a stateless service. </para>
    /// </summary>
    public sealed class StatelessServiceDescription : ServiceDescription
    {
        /// <summary>
        /// <para>Initializes an instance of the <see cref="System.Fabric.Description.StatelessServiceDescription" /> class.</para>
        /// </summary>
        public StatelessServiceDescription()
            : base(ServiceDescriptionKind.Stateless)
        {
            this.InstanceCount = 1;
        }

        internal StatelessServiceDescription(StatelessServiceDescription other)
            : base(other)
        {
            this.InstanceCount = other.InstanceCount;
        }

        /// <summary>
        /// <para>Gets or sets the instance count of this service partition. </para>
        /// </summary>
        /// <value>
        /// <para>The instance count of this service partition. </para>
        /// </value>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.Description.StatelessServiceDescription.InstanceCount" /> property indicates the number of instances to be
        /// created for this service. The specified number of instances will be maintained by Service Fabric. For a partitioned stateless service,
        /// <see cref="System.Fabric.Description.StatelessServiceDescription.InstanceCount" /> indicates the number of instances to be kept per partition.</para>
        /// </remarks>
        public int InstanceCount
        {
            get;
            set;
        }

        internal static new unsafe StatelessServiceDescription CreateFromNative(IntPtr native)
        {
            ReleaseAssert.AssertIfNot(native != IntPtr.Zero, StringResources.Error_NullNativePointer);

            var casted = (NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION*)native;

            var description = new StatelessServiceDescription();

            description.ApplicationName = NativeTypes.FromNativeUri(casted->ApplicationName);
            description.ServiceName = NativeTypes.FromNativeUri(casted->ServiceName);
            description.ServiceTypeName = NativeTypes.FromNativeString(casted->ServiceTypeName);
            description.PartitionSchemeDescription = PartitionSchemeDescription.CreateFromNative(casted->PartitionScheme, casted->PartitionSchemeDescription);
            description.PlacementConstraints = NativeTypes.FromNativeString(casted->PlacementConstraints);
            description.InstanceCount = casted->InstanceCount;
            description.ParseCorrelations(casted->CorrelationCount, casted->Correlations);
            description.ParseLoadMetrics(casted->MetricCount, casted->Metrics);
            description.InitializationData = NativeTypes.FromNativeBytes(casted->InitializationData, casted->InitializationDataSize);

            if (casted->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex1 = (NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1*)casted->Reserved;
            if (ex1 == null)
            {
                throw new ArgumentException(StringResources.Error_UnknownReservedType);
            }

            if (ex1->PolicyList != IntPtr.Zero)
            {
                NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST* pList = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST*)ex1->PolicyList;
                description.ParsePlacementPolicies(pList->PolicyCount, pList->Policies);
            }

            if (ex1->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex2 = (NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2*)ex1->Reserved;
            if (ex2 == null)
            {
                throw new ArgumentException(StringResources.Error_UnknownReservedType);
            }

            if (NativeTypes.FromBOOLEAN(ex2->IsDefaultMoveCostSpecified))
            {
                // This will correctly set the property IsDefaultMoveCostSpecified to true if move cost is valid.
                description.ParseDefaultMoveCost(ex2->DefaultMoveCost);
            }

            if (ex2->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex3 = (NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3*)ex2->Reserved;
            
            description.ServicePackageActivationMode = InteropHelpers.FromNativeServicePackageActivationMode(ex3->ServicePackageActivationMode);
            description.ServiceDnsName = NativeTypes.FromNativeString(ex3->ServiceDnsName);

            if (ex3->Reserved == IntPtr.Zero)
            {
                return description;
            }

            var ex4 = (NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4*)ex3->Reserved;

            if (ex4->ServiceScalingPolicies != IntPtr.Zero && ex4->ScalingPolicyCount > 0)
            {
                description.ParseScalingPolicies(ex4->ScalingPolicyCount, ex4->ServiceScalingPolicies);
            }

            return description;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind)
        {
            this.ValidateDefaultMetricValue();

            var nativeDescription = new NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION[1];
            nativeDescription[0].ApplicationName = pin.AddObject(this.ApplicationName);
            nativeDescription[0].ServiceName = pin.AddObject(this.ServiceName);
            nativeDescription[0].ServiceTypeName = pin.AddBlittable(this.ServiceTypeName);
            nativeDescription[0].PartitionScheme = (NativeTypes.FABRIC_PARTITION_SCHEME)this.PartitionSchemeDescription.Scheme;
            nativeDescription[0].PartitionSchemeDescription = this.PartitionSchemeDescription.ToNative(pin);
            nativeDescription[0].PlacementConstraints = pin.AddBlittable(this.PlacementConstraints);
            nativeDescription[0].InstanceCount = this.InstanceCount;

            var correlations = this.ToNativeCorrelations(pin);
            nativeDescription[0].CorrelationCount = correlations.Item1;
            nativeDescription[0].Correlations = correlations.Item2;

            var initializationData = NativeTypes.ToNativeBytes(pin, this.InitializationData);
            nativeDescription[0].InitializationDataSize = initializationData.Item1;
            nativeDescription[0].InitializationData = initializationData.Item2;

            var metrics = NativeTypes.ToNativeLoadMetrics(pin, this.Metrics);
            nativeDescription[0].MetricCount = metrics.Item1;
            nativeDescription[0].Metrics = metrics.Item2;

            var ex1 = new NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1[1];

            if (this.PlacementPolicies != null)
            {
                var policies = this.ToNativePolicies(pin);
                var pList = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST[1];
                pList[0].PolicyCount = policies.Item1;
                pList[0].Policies = policies.Item2;

                ex1[0].PolicyList = pin.AddBlittable(pList);
            }

            var ex2 = new NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2[1];
            ex2[0].DefaultMoveCost = this.ToNativeDefaultMoveCost();
            ex2[0].IsDefaultMoveCostSpecified = NativeTypes.ToBOOLEAN(this.IsDefaultMoveCostSpecified);

            var ex3 = new NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3[1];
            ex3[0].ServicePackageActivationMode = InteropHelpers.ToNativeServicePackageActivationMode(this.ServicePackageActivationMode);
            ex3[0].ServiceDnsName = pin.AddBlittable(this.ServiceDnsName);

            var ex4 = new NativeTypes.FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4[1];

            if (this.ScalingPolicies != null)
            {
                var scalingPolicies = this.ToNativeScalingPolicies(pin);
                ex4[0].ScalingPolicyCount = scalingPolicies.Item1;
                ex4[0].ServiceScalingPolicies = scalingPolicies.Item2;
            }

            ex3[0].Reserved = pin.AddBlittable(ex4);
            ex2[0].Reserved = pin.AddBlittable(ex3);
            ex1[0].Reserved = pin.AddBlittable(ex2);
            nativeDescription[0].Reserved = pin.AddBlittable(ex1);

            kind = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
            return pin.AddBlittable(nativeDescription);
        }

        internal void ValidateDefaultMetricValue()
        {
            foreach (StatelessServiceLoadMetricDescription metric in this.Metrics)
            {
                if ((metric.Name == "PrimaryCount")
                || (metric.Name == "ReplicaCount")
                || (metric.Name == "SecondaryCount")
                || (metric.Name == "Count" && metric.DefaultLoad != 1)
                || (metric.Name == "InstanceCount" && metric.DefaultLoad != 1))
                {
                    throw new ArgumentException(StringResources.Error_InvalidDefaultMetricValue);
                }
            }      
        }
    }
}
