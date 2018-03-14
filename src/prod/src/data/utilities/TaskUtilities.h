// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Data 
{
    namespace Utilities
    {
        // TODO: Remove template parameter from here
        template<typename T>
        class TaskUtilities
        {
        public:
            static ktl::Awaitable<void> WhenAny(
                __in KSharedArray<ktl::Awaitable<T>>& awaitables, 
                __in KAllocator& Allocator)
            {
                KSharedPtr<KSharedArray<ktl::Awaitable<T>>> tempAwaitables(&awaitables);
                ktl::AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
                NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(Allocator, 0, signalCompletion);

                if (NT_SUCCESS(status) == false)
                {
                    throw ktl::Exception(status);
                }
                for (int i = 0; i < static_cast<int>(tempAwaitables->Count()); i++)
                {
                    AwaitAndSignal(*signalCompletion, *tempAwaitables, i);
                }

                co_await signalCompletion->GetAwaitable();

                co_return;
            }

            // Note: This function must be co_awaited.
            // Ensure to keep the awaitables array alive and unmodified for the duration of the call.
            template<typename T>
            static ktl::Awaitable<void> WhenAll(__in KArray<ktl::Awaitable<T>> const & awaitables)
            {
                // Wait for all operations to drain.
                bool exceptionCaught = false;
                ktl::Exception snappedException = ktl::Exception(STATUS_SUCCESS);
                for (ULONG i = 0; i < awaitables.Count(); i++)
                {
                    try
                    {
                        co_await awaitables[i];
                    }
                    catch (ktl::Exception & exception)
                    {
                        if (exceptionCaught == false)
                        {
                            snappedException = exception;
                            exceptionCaught = true;
                        }
                    }
                }

                if (exceptionCaught)
                {
                    throw snappedException;
                }

                co_return;
            }

            static ktl::Awaitable<NTSTATUS> WhenAll_NoException(__in KArray<ktl::Awaitable<NTSTATUS>> const & awaitables)
            {
                // Wait for all operations to drain.
                NTSTATUS firstError = STATUS_SUCCESS;

                for (ULONG i = 0; i < awaitables.Count(); i++)
                {
                    if (firstError == STATUS_SUCCESS)
                    {
                        firstError = co_await awaitables[i];
                    }
                    else
                    {
                        co_await awaitables[i];
                    }
                }

                co_return firstError;
            }

        private:
            static ktl::Task AwaitAndSignal(
                __in ktl::AwaitableCompletionSource<bool>& signalCompletion,
                __in KSharedArray<ktl::Awaitable<T>>& awaitables, 
                __in int index)
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

                tempCompletion->SetResult(true);
                co_return;
            }
        };
    }
}
