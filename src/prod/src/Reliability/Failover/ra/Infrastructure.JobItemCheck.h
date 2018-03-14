// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class JobItemCheck
            {
            public:

                enum Enum
                {
                    None = 0x0000,
                    RAIsOpen = 0x0001,
                    FTIsNotNull = 0x0002,
                    FTIsOpen = 0x0004,
                    RAIsOpenOrClosing = 0x0008,

                    Default = RAIsOpen | FTIsNotNull,
                    DefaultAndOpen = RAIsOpen | FTIsNotNull | FTIsOpen
                };
            };
        }
    }
}

