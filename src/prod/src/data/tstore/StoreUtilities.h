// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      class StoreUtilities
      {
      public:
         template <typename T>
         static LONG32 GetIndex(const KSharedArray<KSharedPtr<T>>& array, T& item)
         {
            KSharedPtr<T> itemSPtr(&item);
            for (ULONG32 index = 0; index < array.Count(); index++)
            {
               if (array[index].RawPtr() == itemSPtr.RawPtr())
               {
                  return index;
               }
            }

            return -1;
         }

         template <typename T>
         static LONG32 GetIndex(KArray<T>& array, const T& item)
         {
            for (ULONG32 index = 0; index < array.Count(); index++)
            {
               if (array[index] == item)
               {
                  return index;
               }
            }

            return -1;
         }

         template <typename T>
         static bool ContainsKey(const KSharedArray<KSharedPtr<T>>& array, T& item)
         {
            KSharedPtr<T> itemSPtr(&item);
            for (ULONG32 index = 0; index < array.Count(); index++)
            {
               if (array[index].RawPtr() == itemSPtr.RawPtr())
               {
                  return true;
               }
            }

            return false;
         }
        
         template <typename T>
         static ktl::Awaitable<void> WhenAll(
             __in KSharedArray<ktl::Awaitable<T>> & awaitables,
             __in KAllocator& allocator)
         {
             KSharedPtr<KSharedArray<ktl::Awaitable<T>>> tempAwaitables(&awaitables);
             ktl::AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
             NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(allocator, 0, signalCompletion);

             if (NT_SUCCESS(status) == false)
             {
                 throw ktl::Exception(status);
             }

             LONG64 counter = 0;
             for (int i = 0; i < static_cast<int>(tempAwaitables->Count()); i++)
             {
                 AwaitAndSignal(*signalCompletion, *tempAwaitables, i, counter);
             }

             co_await signalCompletion->GetAwaitable();

             co_return;
         }

      private:
         template <typename T>
         static ktl::Task AwaitAndSignal(
             __in ktl::AwaitableCompletionSource<bool>& signalCompletion,
             __in KSharedArray<ktl::Awaitable<T>>& awaitables,
             __in int index,
             __in LONG64 & counter)
         {
             ktl::AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
             KSharedPtr<KSharedArray<ktl::Awaitable<T>>> tempAwaitables(&awaitables);

             try
             {
                 co_await (*tempAwaitables)[index];
             }
             catch (ktl::Exception const & e)
             {
                 tempCompletion->SetException(e);
                 co_return;
             }

             InterlockedIncrement64(&counter);
             if (counter == tempAwaitables->Count())
             {
                 tempCompletion->SetResult(true);
             }

             co_return;
         }
      };
   }
}
