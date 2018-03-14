// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace TraceSinkType
    {
        enum Enum : UCHAR
        {
            ETW = 0,
            TextFile = 1,
            Console = 2
        };
    }
}
