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
        // Lock modes supported by the state manager.
        //
        namespace StateManagerLockMode
        {
            enum Enum
            {
                Read = 0,

                Write = 1,
            };
        }
    }
}
