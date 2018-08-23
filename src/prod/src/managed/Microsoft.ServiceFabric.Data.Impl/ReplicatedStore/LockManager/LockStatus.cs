// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Enumerates supported lock request status values.
    /// </summary>
    internal enum LockStatus
    {
        Invalid,
        Granted,
        Timeout,
        Pending
    }

    /// <summary>
    /// Enumerates supported unlock request status values.
    /// </summary>
    internal enum UnlockStatus
    {
        Invalid,
        Success,
        UnknownResource,
        NotGranted
    }
}