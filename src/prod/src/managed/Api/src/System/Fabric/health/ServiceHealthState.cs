// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of a service, which contains the service identifier and its aggregated health state.</para>
    /// </summary>
    public sealed class ServiceHealthState : EntityHealthState
    {
        internal ServiceHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the service name.</para>
        /// </summary>
        /// <value>
        /// <para>The service name.</para>
        /// </value>
        public Uri ServiceName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the service health state.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.ServiceHealthState"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.ServiceName,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<ServiceHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_SERVICE_HEALTH_STATE_LIST*)nativeListPtr;
                return ServiceHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new ServiceHealthStateList();
            }
        }

        internal static unsafe ServiceHealthState FromNative(NativeTypes.FABRIC_SERVICE_HEALTH_STATE nativeState)
        {
            var serviceHealthState = new ServiceHealthState();
            serviceHealthState.ServiceName = NativeTypes.FromNativeUri(nativeState.ServiceName);
            serviceHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return serviceHealthState;
        }
    }
}