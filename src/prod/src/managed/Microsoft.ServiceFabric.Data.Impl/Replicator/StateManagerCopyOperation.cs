// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Enumeration to indicate what the operation data from the state manager copy stream contains.
    /// </summary>
    internal enum StateManagerCopyOperation : byte
    {
        /// <summary>
        /// The operation data contains the version (int) of the copy protocol.
        /// </summary>
        Version = 0,

        /// <summary>
        /// The operation data contains a chunk of state provider metadata.
        /// </summary>
        StateProviderMetadata,
    }
}