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
        // Operation type for the StateManager.
        //
        namespace OperationType
        {
            enum Enum
            {
                None =      0,
                Add =       1,
                Remove =    2,
                Read =      3
            };
        }
    }
}
