// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    public sealed class StatelessServiceGroup : ServiceGroup
    {
        internal StatelessServiceGroup(
            Uri serviceName,
            string serviceTypeName,
            string serviceManifestVersion,
            HealthState healthState,
            ServiceStatus serviceStatus,
            ServiceGroupMemberList members)
            : base(ServiceKind.Stateless, serviceName, serviceTypeName, serviceManifestVersion, healthState, serviceStatus, members)
        {            
        }

        internal static unsafe StatelessServiceGroup FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_GROUP_QUERY_RESULT_ITEM nativeStatelessServiceGroupQueryResult,
            ServiceGroupMemberList members)
        {
            return new StatelessServiceGroup(
                new Uri(NativeTypes.FromNativeString(nativeStatelessServiceGroupQueryResult.ServiceName)),
                NativeTypes.FromNativeString(nativeStatelessServiceGroupQueryResult.ServiceTypeName),
                NativeTypes.FromNativeString(nativeStatelessServiceGroupQueryResult.ServiceManifestVersion),
                (HealthState)nativeStatelessServiceGroupQueryResult.HealthState,
                (ServiceStatus)nativeStatelessServiceGroupQueryResult.ServiceStatus,
                members);
        }
    }
}