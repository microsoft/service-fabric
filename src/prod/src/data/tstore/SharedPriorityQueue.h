// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SHAREDPRIORITYQUEUE_TAG 'tpQS'

namespace Data 
{
    namespace TStore
    {
        template<typename T>
        class SharedPriorityQueue :
            public KShared<SharedPriorityQueue<T>>,
            public KPriorityQueue<T>
        {
            K_FORCE_SHARED(SharedPriorityQueue)

        public:
            static NTSTATUS
                Create(
                    __in KDelegate<int(__in T const & KeyOne, __in T const & KeyTwo)> compareFunc,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
               result = _new(SHAREDPRIORITYQUEUE_TAG, allocator) SharedPriorityQueue(compareFunc);

               if (result == nullptr)
               {
                  return STATUS_INSUFFICIENT_RESOURCES;
               }

               if (!NT_SUCCESS(result->Status()))
               {
                  // Null Result while fetching failure status with no extra AddRefs or Releases
                  return (SPtr(Ktl::Move(result)))->Status();
               }

               return STATUS_SUCCESS;
            }

        private:
            SharedPriorityQueue(__in KDelegate<int(__in T const & KeyOne, __in T const & KeyTwo)> compareFunc);
        };

        template<typename T>
        SharedPriorityQueue<T>::SharedPriorityQueue(__in KDelegate<int(__in T const & KeyOne, __in T const & KeyTwo)> compareFunc)
            : KPriorityQueue<T>(this->GetThisAllocator(), compareFunc)
        {
        }

        template<typename T>
        SharedPriorityQueue<T>::~SharedPriorityQueue()
        {
        }
    }
}
