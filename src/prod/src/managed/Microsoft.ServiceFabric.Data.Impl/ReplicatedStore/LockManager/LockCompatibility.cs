// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Represents lock compatibility values.
    /// </summary>
    internal enum LockCompatibility
    {
        Conflict = 0,
        NoConflict = 1,
    }
}