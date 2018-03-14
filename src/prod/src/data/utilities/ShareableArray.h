// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SHAREABLEARRAY_TAG 'shAR'

namespace Data
{
    namespace Utilities
    {
        //
        // A KArray class which is also KShared
        // 
        // NOTE: Alternative to KSharedArray that includes constructors for count
        //
        
        template<typename T>
        class ShareableArray
            : public KObject<ShareableArray<T>>
            , public KShared<ShareableArray<T>>
            , public KArray<T>
        {
            K_FORCE_SHARED(ShareableArray)
        public:
            //
            //  Parameters:
            //    ReserveSize           The initial number of elements to reserve (initial internal size of the array)
            //    GrowByPercent         How much to grow the array when it runs out of space
            //                          as percentage of its current size.
            //                          If set to zero, the array will not grow once it is filled to the ReserveSize limit.
            //
            static NTSTATUS Create(
                __in KAllocator& allocator,
                __out SPtr& result,
                __in ULONG ReserveSize = 16,
                __in ULONG GrowByPercent = 50)
            {
                result = _new(SHAREABLEARRAY_TAG, allocator) ShareableArray(ReserveSize, GrowByPercent);

                if (result == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            //
            //  Parameters:
            //    InitialSize           The initial number of elements to reserve (initial internal size of the array)
            //    ItemCount             The initial number of occupied elements
            //    GrowByPercent         How much to grow the array when it runs out of space
            //                          as percentage of its current size.
            //                          If set to zero, the array will not grow once it is filled to the ReserveSize limit.
            //
            static NTSTATUS Create(
                __in KAllocator& allocator,
                __out SPtr& result,
                __in ULONG InitialSize,
                __in ULONG ItemCount,
                __in ULONG GrowByPercent)
            {
                result = _new(SHAREABLEARRAY_TAG, allocator) ShareableArray(InitialSize, ItemCount, GrowByPercent);

                if (result == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                return STATUS_SUCCESS;
            }

        private:
            ShareableArray(
                __in ULONG ReserveSize,
                __in ULONG GrowByPercent);

            ShareableArray(
                __in ULONG InitialSize,
                __in ULONG ItemCount,
                __in ULONG GrowByPercent);
        };

        template<typename T>
        ShareableArray<T>::ShareableArray(
            __in ULONG ReserveSize, 
            __in ULONG GrowByPercent) : KArray<T>(this->GetThisAllocator(), ReserveSize, GrowByPercent)
        {
        }

        template<typename T>
        ShareableArray<T>::ShareableArray(
            __in ULONG InitialSize, 
            __in ULONG ItemCount,
            __in ULONG GrowByPercent) : KArray<T>(this->GetThisAllocator(), InitialSize, ItemCount, GrowByPercent)
        {
        }

        template<typename T>
        ShareableArray<T>::~ShareableArray()
        {
        }
    }
}
