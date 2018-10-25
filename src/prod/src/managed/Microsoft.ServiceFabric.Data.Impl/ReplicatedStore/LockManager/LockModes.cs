// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Lock modes supported by the lock manager.
    /// </summary>
    internal enum LockMode : byte
    {
        Free,
        SchemaStability,
        SchemaModification,
        Shared,
        Update,
        Exclusive,
        IntentShared,
        IntentExclusive,
    }
}