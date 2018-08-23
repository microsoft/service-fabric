// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Represents the status of a service retrieved by calling <see cref="System.Fabric.FabricClient.QueryClient.GetServiceListAsync(System.Uri)" />.
    /// </summary>
    public enum ServiceStatus
    {
        /// <summary>
        /// The service status is not yet known.
        /// </summary>
        Unknown = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_UNKNOWN,
        
        /// <summary>
        /// The service has been successfully created.
        /// </summary>
        Active = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_ACTIVE,
        
        /// <summary>
        /// The service is being upgraded.
        /// </summary>
        Upgrading = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_UPGRADING,
        
        /// <summary>
        /// The service is being deleted.
        /// </summary>
        Deleting = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_DELETING,

        /// <summary>
        /// The service is being created.
        /// </summary>
        Creating = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_CREATING,

        /// <summary>
        /// Creation or deletion was terminated due to persistent failures.
        /// </summary>
        Failed = NativeTypes.FABRIC_QUERY_SERVICE_STATUS.FABRIC_QUERY_SERVICE_STATUS_FAILED
    }
}