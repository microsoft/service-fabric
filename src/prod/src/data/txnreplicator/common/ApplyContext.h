// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Provided to the state manager as part of the ApplyAsync API call
    // This is an enum with the first 16 bits representing the role of the replica and the last 16 bits representing the kind of operation (redo/undo/unlock)
    //
    // State Manager can use this information to take different control paths depending on whether the replica is in primary/secondary/recovery role
    //
    namespace ApplyContext
    {
        enum Enum
        {
            /// <summary>
            /// Applier is in invalid state
            /// </summary>
            Invalid = 0,

            /// <summary>
            /// The replica role is Primary
            /// </summary>
            PRIMARY = 0x1 << 16,

            /// <summary>
            /// The replica role is Active Secondary
            /// </summary>
            SECONDARY = 0x2 << 16,

            /// <summary>
            /// The replicator is performing a local recovery
            /// </summary>
            RECOVERY = 0x4 << 16,

            /// <summary>
            /// Role mask
            /// </summary>
            ROLE_MASK = (-1) << 16,

            /// <summary>
            /// The replicator is replaying 'Redo' data
            /// </summary>
            REDO = 0x2,

            /// <summary>
            /// The replicator is invoking 'Unlock'
            /// </summary>
            UNLOCK = 0x3,

            /// <summary>
            /// The replicator is replaying 'Undo' data
            /// </summary>
            UNDO = 0x4,

            /// <summary>
            /// The replicator is replaying 'false progressed' data
            /// </summary>
            FALSE_PROGRESS = 0x5,

            /// <summary>
            /// Operation mask
            /// </summary>
            OPERATION_MASK = (0x1 << 16) - 1,

            /// <summary>
            /// Applier is Primary and it is a Redo operation
            /// </summary>
            PrimaryRedo = PRIMARY | REDO,

            /// <summary>
            /// Applier is Primary and it is an Undo operation
            /// </summary>
            PrimaryUndo = PRIMARY | UNDO,

            /// <summary>
            /// Applier is Primary and it is a Unlock operation
            /// </summary>
            PrimaryUnlock = PRIMARY | UNLOCK,

            /// <summary>
            /// Applier is a Secondary and it is a Redo operation
            /// </summary>
            SecondaryRedo = SECONDARY | REDO,

            /// <summary>
            /// Applier is a Secondary and it is an Undo operation
            /// </summary>
            SecondaryUndo = SECONDARY | UNDO,

            /// <summary>
            /// Applier is Secondary and it is a Unlock operation
            /// </summary>
            SecondaryUnlock = SECONDARY | UNLOCK,

            /// <summary>
            /// Applier is a Secondary and it is an False Progress operation
            /// </summary>
            SecondaryFalseProgress = SECONDARY | FALSE_PROGRESS,

            /// <summary>
            /// Applier is in Recovery Stage and it is a Redo operation
            /// </summary>
            RecoveryRedo = RECOVERY | REDO,

            /// <summary>
            /// Applier is in Recovery Stage and it is an Undo operation
            /// </summary>
            RecoveryUndo = RECOVERY | UNDO
        };
    }
}
