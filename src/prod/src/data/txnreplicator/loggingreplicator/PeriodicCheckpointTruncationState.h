// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        /*
         Various phases for periodic checkpointing + truncation
         Provides guarantee that log does not contain items older than configured interval
        
         Periodic Checkpoint + Log Head Truncation State Machine

         States:
            
            NotStarted            - Configured interval has not yet passed
            
            Ready                 - Configured interval has passed. 
                                    Subsequent call to 'ShouldCheckpoint' return true (if no checkpoint in progress), overriding checkpointIntervalBytes configuration.
            
            CheckpointStarted     - Checkpoint initiated. 
            
            CheckpointCompleted   - Checkpoint completed. 
                                    Subsequent calls to 'ShouldTruncateHead' return true (if no truncation in progress), overriding truncation threshold and min log size configurations

            TruncationStarted     - Truncation has been initiated. 
        
         Accepted State Transitions:

            NotStarted  ->Ready  ->CheckpointStarted  ->CheckpointCompleted  ->TruncationStarted  ->NotStarted                    | Happy Path, truncation completes and resets state to 'NotStarted'

            NotStarted  ->Ready  ->CheckpointStarted  ->NotStarted  ->Ready                                                       | Checkpoint unable to complete before replica closed and re-opened.
                                                                                                                                    Initial checkpoint time is recovered, ensuring checkpoint timer fires immediately
                                                                                                                                    Group commit is initiated, restarting the process

            NotStarted  ->CheckpointCompleted  ->TruncationStarted  ->NotStarted                                                  | Truncation unable to complete before replica closed and re-opened.
                                                                                                                                    Truncation time < last periodic checkpoint time, state = CheckpointCompleted
                                                                                                                                    Ensures head truncation occurs during next physical record insert
         */
        namespace PeriodicCheckpointTruncationState
        {
            enum Enum
            {
                NotStarted = 0,
                Ready = 1,
                CheckpointStarted = 2,
                CheckpointCompleted = 3,
                TruncationStarted = 4,
                LastValidEnum = TruncationStarted
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(PeriodicCheckpointTruncationState);
        }
    }
}
