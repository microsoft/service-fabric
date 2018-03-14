// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DIFFERENTIALSTATEVERSIONS_TAG 'vsFD'

namespace Data
{
    namespace TStore
    {
        template<typename TValue>
        class DifferentialStateVersions : public KObject<DifferentialStateVersions<TValue>>, public KShared<DifferentialStateVersions<TValue>>
        {
            K_FORCE_SHARED(DifferentialStateVersions)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(DIFFERENTIALSTATEVERSIONS_TAG, allocator) DifferentialStateVersions();

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            __declspec(property(get = get_CurrentVersion)) KSharedPtr<VersionedItem<TValue>> CurrentVersionSPtr;
            KSharedPtr<VersionedItem<TValue>> get_CurrentVersion() const
            {
                return currentVersionSPtr_.Get();
            }

            __declspec(property(get = get_PreviousVersion)) KSharedPtr<VersionedItem<TValue>> PreviousVersionSPtr;
            KSharedPtr<VersionedItem<TValue>> get_PreviousVersion() const
            {
                return previousVersionSPtr_.Get();
            }

            void SetCurrentVersion(__in VersionedItem<TValue>& value)
            {
                currentVersionSPtr_.Put(Ktl::Move(&value));
            }

            void SetPreviousVersion(__in KSharedPtr<VersionedItem<TValue>> value)
            {
                previousVersionSPtr_.Put(Ktl::Move(value));
            }

        private:
            ThreadSafeSPtrCache<VersionedItem<TValue>> currentVersionSPtr_ = {nullptr};
            ThreadSafeSPtrCache<VersionedItem<TValue>> previousVersionSPtr_ = {nullptr};
        };

        template <typename TValue>
        DifferentialStateVersions<TValue>::DifferentialStateVersions()
        {
        }

        template <typename TValue>
        DifferentialStateVersions<TValue>::~DifferentialStateVersions()
        {
        }
    }
}
