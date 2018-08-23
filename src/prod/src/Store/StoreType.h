// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace StoreType
    {
        enum Enum
        {
            Invalid,

            // Local store based on ESE for Windows
            Local,

            // Local store based on TStore
            TStore,

            // SQL store
            Sql,
        };
    }
}
