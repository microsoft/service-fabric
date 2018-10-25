// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents the health policy used to evaluate the health of services belonging to a service type.</para>
    /// </summary>
    public class ServiceTypeHealthPolicy
    {
        private byte maxPercentUnhealthyServices;
        private byte maxPercentUnhealthyPartitionsPerService;
        private byte maxPercentUnhealthyReplicasPerPartition;

        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Health.ServiceTypeHealthPolicy" /> class.</para>
        /// </summary>
        /// <remarks>By default, no errors or warnings are tolerated.</remarks>
        public ServiceTypeHealthPolicy()
        {
            this.MaxPercentUnhealthyServices = 0;
            this.MaxPercentUnhealthyPartitionsPerService = 0;
            this.MaxPercentUnhealthyReplicasPerPartition = 0;
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy services.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the maximum allowed percentage of unhealthy services. 
        /// Allowed values are <see cref="System.Byte" /> values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of services that can be unhealthy 
        /// before the application is considered in error. If the percentage is respected but there is at least one unhealthy service,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy services of the specific service type 
        /// over the total number of services of the specific service type.
        /// The computation rounds up to tolerate one failure on small numbers of services. Default percentage: zero.
        /// </para>
        /// </remarks>
        public byte MaxPercentUnhealthyServices
        {
            get
            {
                return this.maxPercentUnhealthyServices;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyServices = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy partitions per service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the maximum allowed percentage of unhealthy partitions per service.
        /// Allowed values are <see cref="System.Byte" /> values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of partitions that can be unhealthy 
        /// before the service is considered in error. If the percentage is respected but there is at least one unhealthy partition,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy partitions over the total number of partitions in the service.
        /// The computation rounds up to tolerate one failure on small numbers of partitions. Default percentage: zero.
        /// </para>
        /// </remarks>
        public byte MaxPercentUnhealthyPartitionsPerService
        {
            get
            {
                return this.maxPercentUnhealthyPartitionsPerService;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyPartitionsPerService = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy replicas per partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the maximum allowed percentage of unhealthy replicas per partition.
        /// Allowed values are <see cref="System.Byte" /> values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of replicas that can be unhealthy 
        /// before the partition is considered in error. If the percentage is respected but there is at least one unhealthy replica,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy replicas over the total number of replicas in the partition.
        /// The computation rounds up to tolerate one failure on small numbers of replicas. Default percentage: zero.
        /// </para>
        /// </remarks>
        public byte MaxPercentUnhealthyReplicasPerPartition
        {
            get
            {
                return this.maxPercentUnhealthyReplicasPerPartition;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyReplicasPerPartition = value;
            }
        }

        /// <summary>
        /// Gets a string representation of the service type health policy.
        /// </summary>
        /// <returns>A string representation of the service type health policy.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "ServiceTypeHealthPolicy: [MaxPercentUnhealthyServices={0}, MaxPercentUnhealthyPartitionsPerService={1}, MaxPercentUnhealthyReplicasPerPartition={2}]",
                this.MaxPercentUnhealthyServices,
                this.MaxPercentUnhealthyPartitionsPerService,
                this.MaxPercentUnhealthyReplicasPerPartition);
        }

        internal static bool AreEqualWithNullAsDefault(ServiceTypeHealthPolicy current, ServiceTypeHealthPolicy other)
        {
            if ((current == null) && (other == null))
            {
                return true;
            }

            var currentOrDefault = (current == null) ? new ServiceTypeHealthPolicy() : current;
            var otherOrDefault = (other == null) ? new ServiceTypeHealthPolicy() : other;
            return AreEqual(currentOrDefault, otherOrDefault);
        }

        internal static bool AreEqual(ServiceTypeHealthPolicy current, ServiceTypeHealthPolicy other)
        {
            if ((current != null) && (other != null))
            {
                return (current.MaxPercentUnhealthyServices == other.MaxPercentUnhealthyServices) &&
                    (current.MaxPercentUnhealthyPartitionsPerService == other.MaxPercentUnhealthyPartitionsPerService) &&
                    (current.MaxPercentUnhealthyReplicasPerPartition == other.MaxPercentUnhealthyReplicasPerPartition);
            }
            else
            {
                return (current == null) && (other == null);
            }
        }

        internal static unsafe ServiceTypeHealthPolicy FromNative(IntPtr nativeServiceTypeHealthPolicyPtr)
        {
            if (nativeServiceTypeHealthPolicyPtr == IntPtr.Zero)
            {
                return null;
            }

            var nativeServiceTypeHealthPolicy = (NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY*)nativeServiceTypeHealthPolicyPtr;

            var serviceTypeHealthPolicy = new ServiceTypeHealthPolicy();
            serviceTypeHealthPolicy.MaxPercentUnhealthyServices = nativeServiceTypeHealthPolicy->MaxPercentUnhealthyServices;
            serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = nativeServiceTypeHealthPolicy->MaxPercentUnhealthyPartitionsPerService;
            serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = nativeServiceTypeHealthPolicy->MaxPercentUnhealthyReplicasPerPartition;

            return serviceTypeHealthPolicy;
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeServiceTypeHealthPolicy = new NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY();

            nativeServiceTypeHealthPolicy.MaxPercentUnhealthyServices = this.MaxPercentUnhealthyServices;
            nativeServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = this.MaxPercentUnhealthyPartitionsPerService;
            nativeServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = this.MaxPercentUnhealthyReplicasPerPartition;

            return pin.AddBlittable(nativeServiceTypeHealthPolicy);
        }

        internal static unsafe void FromNativeMap(IntPtr nativeServiceTypeHealthPolicyMapPtr, IDictionary<string, ServiceTypeHealthPolicy> map)
        {
            if (nativeServiceTypeHealthPolicyMapPtr == IntPtr.Zero)
            {
                return;
            }

            var nativeServiceTypeHealthPolicyMap = (NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP*)nativeServiceTypeHealthPolicyMapPtr;
            var offset = Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM));
            for (int i = 0; i < nativeServiceTypeHealthPolicyMap->Count; i++)
            {
                var nativeMapItemPtr = nativeServiceTypeHealthPolicyMap->Items + (i * offset);
                var nativeMapItem = (NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM*)nativeMapItemPtr;

                var serviceTypeName = NativeTypes.FromNativeString(nativeMapItem->ServiceTypeName);
                var serviceTypeHealthPolicy = ServiceTypeHealthPolicy.FromNative(nativeMapItem->ServiceTypeHealthPolicy);

                map.Add(serviceTypeName, serviceTypeHealthPolicy);
            }
        }

        internal static unsafe IntPtr ToNativeMap(PinCollection pin, IDictionary<string, ServiceTypeHealthPolicy> map)
        {
            if (map.Count > 0)
            {
                var nativeServiceTypeHealthPolicyMap = new NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP();

                var nativeArray = new NativeTypes.FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM[map.Count];

                int ix = 0;
                foreach (var pair in map)
                {
                    nativeArray[ix].ServiceTypeName = pin.AddObject(pair.Key);
                    nativeArray[ix].ServiceTypeHealthPolicy = pair.Value.ToNative(pin);
                    ++ix;
                }

                nativeServiceTypeHealthPolicyMap.Count = (uint)nativeArray.Length;
                nativeServiceTypeHealthPolicyMap.Items = pin.AddBlittable(nativeArray);

                return pin.AddBlittable(nativeServiceTypeHealthPolicyMap);
            }
            else
            {
                return IntPtr.Zero;
            }
        }
    }
}