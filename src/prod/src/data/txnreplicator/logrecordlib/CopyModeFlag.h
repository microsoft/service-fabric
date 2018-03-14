// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        namespace CopyModeFlag
        {
            enum Enum
            {
                Invalid = 0x0,

                FalseProgress = 0x01,

                None = 0x02,

                Partial = 0x04,

                Full = 0x08,
            };
        }
    }
}
