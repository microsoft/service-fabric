// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Lock modes supported by the state manager.
    /// </summary>
    internal enum StateManagerLockMode : byte
    {
        Read,

        Write,
    }
}