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
        // Different stages of a checkpoint
        //
        namespace CheckpointState
        {
            enum Enum
            {
                Invalid = 0,

                // Flushed and ready to invoke PrepareCheckpoint()
                Ready = 1,
                
                // Checkpoint Lsn is stable and can hence call PerformCheckpointAsync()
                Applied = 2,

                // Checkpoint finished executing PerformCheckpointAsync()
                Completed = 3,

                // Either prepare or perform checkpoint failed
                Faulted = 4,
                
                // Checkpoint was cancelled during close
                Aborted = 5,

                LastValidEnum = Aborted
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(CheckpointState);
        }
    }
}
