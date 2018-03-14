// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        namespace LogRecordType
        {
            enum Enum
            {
                Invalid = 0,

                // Logical log records
                BeginTransaction = 1,

                Operation = 2,

                EndTransaction = 3,

                Barrier = 4,

                UpdateEpoch = 5,

                Backup = 6,

                // Physical log records
                BeginCheckpoint = 7,

                EndCheckpoint = 8,

                Indexing = 9,

                TruncateHead = 10,

                TruncateTail = 11,

                Information = 12,

                CompleteCheckpoint = 13,

                LastValidEnum = CompleteCheckpoint
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(LogRecordType);
        }
    }
}
