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
    /// <para>Represents a stateful service.</para>
    /// </summary>
    public sealed class StatefulService : Service
    {
        internal StatefulService(
            Uri serviceName,
            string serviceTypeName,
            string serviceManifestVersion,
            bool hasPersistedState,
            HealthState healthState,
            ServiceStatus serviceStatus,
            bool isServiceGroup = false)
            : base(ServiceKind.Stateful, serviceName, serviceTypeName, serviceManifestVersion, healthState, serviceStatus, isServiceGroup)
        {
            this.HasPersistedState = hasPersistedState;
        }

        internal StatefulService()
            : base(ServiceKind.Stateful, null, null, null, HealthState.Invalid, ServiceStatus.Unknown)
        {
        }

        /// <summary>
        /// <para>Gets a value that determines whether the current service has persisted state.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the current service has persisted state; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool HasPersistedState { get; private set; }

        internal static unsafe StatefulService FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM nativeStatefulServiceQueryResult)
        {
            ServiceStatus serviceStatus = ServiceStatus.Unknown;
            bool isServiceGroup = false;
            if (nativeStatefulServiceQueryResult.Reserved != IntPtr.Zero)
            {
                var nativeStatefulServiceQueryResultEx1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1*)nativeStatefulServiceQueryResult.Reserved;
                if (nativeStatefulServiceQueryResultEx1 == null)
                {
                    throw new ArgumentException(StringResources.Error_UnknownReservedType);
                }

                serviceStatus = (ServiceStatus)nativeStatefulServiceQueryResultEx1->ServiceStatus;

                if (nativeStatefulServiceQueryResultEx1->Reserved != IntPtr.Zero)
                {
                    var nativeStatefulServiceQueryResultEx2 = (NativeTypes.FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX2*)nativeStatefulServiceQueryResultEx1->Reserved;

                    isServiceGroup = NativeTypes.FromBOOLEAN(nativeStatefulServiceQueryResultEx2->IsServiceGroup);
                }
            }

            return new StatefulService(
                new Uri(NativeTypes.FromNativeString(nativeStatefulServiceQueryResult.ServiceName)),
                NativeTypes.FromNativeString(nativeStatefulServiceQueryResult.ServiceTypeName),
                NativeTypes.FromNativeString(nativeStatefulServiceQueryResult.ServiceManifestVersion),
                NativeTypes.FromBOOLEAN(nativeStatefulServiceQueryResult.HasPersistedState),
                (HealthState)nativeStatefulServiceQueryResult.HealthState,
                serviceStatus,
                isServiceGroup);
        }
    }
}