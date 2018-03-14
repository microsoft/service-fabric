// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // Enumeration to indicate what the operation data from the state manager copy stream contains.
        //
        enum StateManagerCopyOperation : byte
        {
            // The operation data contains the version (ULONG32) of the copy protocol.
            Version = 0,

            // The operation data contains a chunk of state provider metadata.
            StateProviderMetadata
        };
    }
}
