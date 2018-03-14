// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        enum ReadMode : byte
        {
            Off = 0,

            ReadValue = 1,

            CacheResult = 2
        };
    }
}
