// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents a stateless service.</para>
    /// </summary>
    public sealed class StatelessService : Service
    {
        internal StatelessService(
            Uri serviceName,
            string serviceTypeName,
            string serviceManifestVersion,
            HealthState healthState,
            ServiceStatus serviceStatus,
            bool isServiceGroup = false)
            : base(ServiceKind.Stateless, serviceName, serviceTypeName, serviceManifestVersion, healthState, serviceStatus, isServiceGroup)
        {
        }

        internal StatelessService()
            : base(ServiceKind.Stateless, null, null, null, HealthState.Invalid, ServiceStatus.Unknown)
        {
        }

        internal static unsafe StatelessService FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM nativeStatelessServiceQueryResult)
        {
            ServiceStatus serviceStatus = ServiceStatus.Unknown;
            bool isServiceGroup = false;
            if (nativeStatelessServiceQueryResult.Reserved != IntPtr.Zero)
            {
                var nativeStatelessServiceQueryResultEx1 = (NativeTypes.FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1*)nativeStatelessServiceQueryResult.Reserved;
                if (nativeStatelessServiceQueryResultEx1 == null)
                {
                    throw new ArgumentException(StringResources.Error_UnknownReservedType);
                }

                serviceStatus = (ServiceStatus)nativeStatelessServiceQueryResultEx1->ServiceStatus;

                if (nativeStatelessServiceQueryResultEx1->Reserved != IntPtr.Zero)
                {
                    var nativeStatelessServiceQueryResultEx2 = (NativeTypes.FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX2*)nativeStatelessServiceQueryResultEx1->Reserved;

                    isServiceGroup = NativeTypes.FromBOOLEAN(nativeStatelessServiceQueryResultEx2->IsServiceGroup);
                }
            }

            return new StatelessService(
                new Uri(NativeTypes.FromNativeString(nativeStatelessServiceQueryResult.ServiceName)),
                NativeTypes.FromNativeString(nativeStatelessServiceQueryResult.ServiceTypeName),
                NativeTypes.FromNativeString(nativeStatelessServiceQueryResult.ServiceManifestVersion),
                (HealthState)nativeStatelessServiceQueryResult.HealthState,
                serviceStatus,
                isServiceGroup);
        }
    }
}