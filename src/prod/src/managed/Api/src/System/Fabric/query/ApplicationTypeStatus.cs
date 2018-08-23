// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Specifies the status of the application type.
    /// </summary>
    public enum ApplicationTypeStatus
    {
        /// <summary>
        /// Invalid.
        /// </summary>
        Invalid = NativeTypes.FABRIC_APPLICATION_TYPE_STATUS.FABRIC_APPLICATION_TYPE_STATUS_INVALID,

        /// <summary>
        /// Currently being provisioned.
        /// </summary>
        Provisioning = NativeTypes.FABRIC_APPLICATION_TYPE_STATUS.FABRIC_APPLICATION_TYPE_STATUS_PROVISIONING,
        
        /// <summary>
        /// Currently available for use.
        /// </summary>
        Available = NativeTypes.FABRIC_APPLICATION_TYPE_STATUS.FABRIC_APPLICATION_TYPE_STATUS_AVAILABLE,

        /// <summary>
        /// Currently being unprovisioned.
        /// </summary>
        Unprovisioning = NativeTypes.FABRIC_APPLICATION_TYPE_STATUS.FABRIC_APPLICATION_TYPE_STATUS_UNPROVISIONING,

        /// <summary>
        /// Unavailable for use due to provisioning failure.
        /// </summary>
        Failed = NativeTypes.FABRIC_APPLICATION_TYPE_STATUS.FABRIC_APPLICATION_TYPE_STATUS_FAILED
    }
}

