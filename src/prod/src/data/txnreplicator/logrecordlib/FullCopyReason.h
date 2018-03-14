// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        namespace FullCopyReason
        {
            enum Enum
            {
                Invalid = 0,

                DataLoss = 1,

                InsufficientLogs = 2,

                AtomicRedoOperationFalseProgressed = 3,

                Other = 4,

                ProgressVectorTrimmed = 5,

                ValidationFailed = 6,

                LastValidEnum = ValidationFailed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(FullCopyReason);
        }
    }
}
