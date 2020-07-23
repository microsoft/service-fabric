// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    /// <summary>
    /// Data reading Mode
    /// </summary>
    public enum DataReaderMode
    {
        // We are reading from Cache
        CacheMode,

        // We are directly reading from the source.
        DirectReadMode
    }
}
