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
        // Various phases for log head truncation
        //
        namespace TruncationState
        {
            enum Enum
            {
                Invalid = 0,
                
                // TruncateHead log record is flushed to disk successfully
                Ready = 1,

                // TruncateHead log record lsn is stable is quorum ack'd and is waiting to be completed
                // Completion can be blocked if there are pending log readers behind the truncation point in the log
                Applied = 2,

                Completed = 3,

                // TruncateHead log record flush failed
                Faulted = 4,

                // Truncation was aborted because of a close on the replica
                Aborted = 5,

                LastValidEnum = Aborted
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(TruncationState);
        }
    }
}
