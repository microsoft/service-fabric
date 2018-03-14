// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DEFAULT_COMPARER_TAG 'moCD'

namespace Data
{
    namespace Utilities
    {
        template<typename T>
        class DefaultComparer
            : public KObject<DefaultComparer<T>>
            , public KShared<DefaultComparer<T>>
            , public IComparer<T>
        {
            K_FORCE_SHARED(DefaultComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out KSharedPtr<IComparer<T>> & result)
            {
                SPtr output = _new(DEFAULT_COMPARER_TAG, allocator) DefaultComparer();
                if (output == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(output->Status()))
                {
                    return output->Status();
                }

                result = output.RawPtr();
                return STATUS_SUCCESS;
            }

            int Compare(__in const T & lhs, __in const T & rhs) const override
            {
                return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
            }
        };

        template<typename T>
        DefaultComparer<T>::DefaultComparer()
        {
        }

        template<typename T>
        DefaultComparer<T>::~DefaultComparer()
        {
        }
    }
}
