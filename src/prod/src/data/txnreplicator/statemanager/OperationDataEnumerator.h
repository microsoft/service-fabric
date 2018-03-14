// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class OperationDataEnumerator:
            public IEnumerator<Utilities::OperationData::CSPtr>
        {
            K_FORCE_SHARED(OperationDataEnumerator)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result);

            Utilities::OperationData::CSPtr Current() override;

            bool MoveNext() override;

            NTSTATUS Add(__in KBuffer const & buffer);

        private:
            KArray<KBuffer::CSPtr> bufferCollection_;

            LONG32 currentIndex_ = -1;

            // OperationDataEnumerator has two phase, add phase and read phase,
            // read phase only happened after the add phase.
            // Add should assert if add operation gets called after isReadCalled_ sets to true.
            bool isReadCalled_ = false;
        };
    }
}
