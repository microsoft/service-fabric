// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        // Note: Using class becuase of C2: OpenParameters has an invariant.
        class OpenParameters final :
            public KObject<OpenParameters>,
            public KShared<OpenParameters>
        {
            K_FORCE_SHARED(OpenParameters)

        public:
            static NTSTATUS Create(
                __in bool completeCheckpoint,
                __in bool cleanupRestore,
                __in KAllocator& allocator,
                __out CSPtr & result) noexcept;

            __declspec(property(get = get_CompleteCheckpoint)) bool CompleteCheckpoint;
            bool OpenParameters::get_CompleteCheckpoint() const { return completeCheckpoint_; }

            __declspec(property(get = get_CleanupRestore)) bool CleanupRestore;
            bool OpenParameters::get_CleanupRestore() const { return cleanupRestore_; }

        private:
            NOFAIL OpenParameters(
                __in bool completeCheckpoint,
                __in bool cleanupRestor) noexcept;

        private:
            bool const completeCheckpoint_;
            bool const cleanupRestore_;
        };
    }
}
