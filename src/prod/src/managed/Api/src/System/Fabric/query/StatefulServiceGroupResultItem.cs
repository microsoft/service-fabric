// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    public sealed class StatefulServiceGroup : ServiceGroup
    {
        internal StatefulServiceGroup(
            Uri serviceName,
            string serviceTypeName,
            string serviceManifestVersion,
            bool hasPersistedState,
            HealthState healthState,
            ServiceStatus serviceStatus,
            ServiceGroupMemberList members)
            : base(ServiceKind.Stateful, serviceName, serviceTypeName, serviceManifestVersion, healthState, serviceStatus, members)
        {
            this.HasPersistedState = hasPersistedState;
        }

        public bool HasPersistedState { get; private set; }

        internal static unsafe StatefulServiceGroup FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_GROUP_QUERY_RESULT_ITEM nativeStatefulServiceGroupQueryResult,
            ServiceGroupMemberList members)
        {
            return new StatefulServiceGroup(
                new Uri(NativeTypes.FromNativeString(nativeStatefulServiceGroupQueryResult.ServiceName)),
                NativeTypes.FromNativeString(nativeStatefulServiceGroupQueryResult.ServiceTypeName),
                NativeTypes.FromNativeString(nativeStatefulServiceGroupQueryResult.ServiceManifestVersion),
                NativeTypes.FromBOOLEAN(nativeStatefulServiceGroupQueryResult.HasPersistedState),
                (HealthState)nativeStatefulServiceGroupQueryResult.HealthState,
                (ServiceStatus)nativeStatefulServiceGroupQueryResult.ServiceStatus,
                members);
        }
    }
}