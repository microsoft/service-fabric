// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Specifies the status of the application.
    /// </summary>
    public enum ApplicationStatus
    {
        /// <summary>
        /// Invalid.
        /// </summary>
        Invalid = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_INVALID,

        /// <summary>
        /// Ready.
        /// </summary>
        Ready = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_READY,
        
        /// <summary>
        /// Currently being upgraded.
        /// </summary>
        Upgrading = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_UPGRADING,

        /// <summary>
        /// Currently being created.
        /// </summary>
        Creating = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_CREATING,

        /// <summary>
        /// Currently being deleted.
        /// </summary>
        Deleting = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_DELETING,

        /// <summary>
        /// Creation or deletion was terminated due to persistent failures.
        /// </summary>
        Failed = NativeTypes.FABRIC_APPLICATION_STATUS.FABRIC_APPLICATION_STATUS_FAILED
    }
}
