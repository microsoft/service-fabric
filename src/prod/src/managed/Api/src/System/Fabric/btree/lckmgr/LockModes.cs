// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.LockManager
{
    using System;

    /// <summary>
    /// Lock modes supported by the lock manager.
    /// </summary>
    public enum LockMode
    {
        Free,
        SchemaStability,
        SchemaModification,
        Shared,
        Update,
        Exclusive,
        IntentShared,
        IntentUpdate,
        IntentExclusive,
        SharedIntentUpdate,
        SharedIntentExclusive,
        UpdateIntentExclusive,
    }
}