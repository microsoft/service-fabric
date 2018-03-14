// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // Contains the recovered or full copied checkpoint lsn
        //
        class RecoveredOrCopiedCheckpointState final
            : public KObject<RecoveredOrCopiedCheckpointState>
            , public KShared<RecoveredOrCopiedCheckpointState>
        {
            K_FORCE_SHARED(RecoveredOrCopiedCheckpointState)

        public:

            static RecoveredOrCopiedCheckpointState::SPtr Create(__in KAllocator & allocator);

            __declspec(property(get = get_Lsn)) LONG64 Lsn;
            LONG64 get_Lsn() const
            {
                return sequenceNumber_;
            }

            void Update(LONG64 lsn);

        private:
            
            LONG64 sequenceNumber_;
        };
    }
}
