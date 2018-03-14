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
        // Const Serialized buffers that never change in the replicator
        // These are created once by the loggigngreplicator and are passed into the copy stream for them to use when they need to add the copy stage information
        //
        class CopyStageBuffers final
            : public KObject<CopyStageBuffers>
            , public KShared<CopyStageBuffers>
        {
            K_FORCE_SHARED(CopyStageBuffers)

        public:
            static CopyStageBuffers::SPtr Create(__in KAllocator & allocator);

            __declspec(property(get = get_CopyFalseProgressOperation)) KBuffer::SPtr CopyFalseProgressOperation;
            KBuffer::SPtr get_CopyFalseProgressOperation() const
            {
                return copyFalseProgressOperation_;
            }

            __declspec(property(get = get_CopyLogOperation)) KBuffer::SPtr CopyLogOperation;
            KBuffer::SPtr get_CopyLogOperation() const
            {
                return copyLogOperation_;
            }

            __declspec(property(get = get_CopyProgressVectorOperation)) KBuffer::SPtr CopyProgressVectorOperation;
            KBuffer::SPtr get_CopyProgressVectorOperation() const
            {
                return copyProgressVectorOperation_;
            }

            __declspec(property(get = get_CopyStateOperation)) KBuffer::SPtr CopyStateOperation;
            KBuffer::SPtr get_CopyStateOperation() const
            {
                return copyStateOperation_;
            }
        private:
            CopyStageBuffers(__in KAllocator & allocator);

            KBuffer::SPtr copyFalseProgressOperation_;
            KBuffer::SPtr copyLogOperation_;
            KBuffer::SPtr copyProgressVectorOperation_;
            KBuffer::SPtr copyStateOperation_;
        };
    }
}
