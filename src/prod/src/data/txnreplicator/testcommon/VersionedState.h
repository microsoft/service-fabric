// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class VersionedState
            : public KObject<VersionedState>
            , public KShared<VersionedState>
        {
            K_FORCE_SHARED(VersionedState);

        public:
            static VersionedState::CSPtr Create(
                __in FABRIC_SEQUENCE_NUMBER version,
                __in KString const & value,
                __in KAllocator & allocator) noexcept;

        public:
            // Expose the IsStateChanged property for testing
            __declspec(property(get = get_Version)) FABRIC_SEQUENCE_NUMBER Version;
            FABRIC_SEQUENCE_NUMBER get_Version() const;

            __declspec(property(get = get_Value)) KString::CSPtr Value;
            KString::CSPtr get_Value() const;

        private:
            VersionedState(
                __in FABRIC_SEQUENCE_NUMBER version,
                __in KString const & value);

        private:
            FABRIC_SEQUENCE_NUMBER const version_;
            KString::CSPtr const value_;
        };
    }
}
