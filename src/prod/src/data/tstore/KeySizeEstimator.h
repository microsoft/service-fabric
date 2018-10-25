// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEY_SIZE_ESTIMATOR_TAG 'meSK'

namespace Data
{
    namespace TStore
    {
        class KeySizeEstimator
            : public KObject<KeySizeEstimator>
            , public KShared<KeySizeEstimator>
        {
            K_FORCE_SHARED(KeySizeEstimator)
        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result);

            void AddKeySize(__in LONG64 totalSize, __in LONG64 count=1);

            void SetInitialEstimate(__in LONG64 estimate);
            LONG64 GetEstimatedKeySize();
    
        private:
            LONG64 initialEstimate_ = -1;
            LONG64 numKeys_ = 0;
            LONG64 totalSize_ = 0;
        };
    }
}
