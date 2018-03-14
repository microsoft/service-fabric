// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // Indicates the kind of reader that has an open handle on the log file
        //
        namespace LogReaderType
        {
            enum Enum
            {
                Default = 0,

                Recovery = 1,

                PartialCopy = 2,

                FullCopy = 3,

                Backup = 4
            };
        }
    }
}
