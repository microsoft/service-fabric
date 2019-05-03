//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#define SWEEP_MANAGER_TAG 'swMG'

namespace Data
{
   namespace TStore
   {
      template <typename TKey, typename TValue>
      class SweepManager
         : public KObject<SweepManager<TKey, TValue>>
         , public KShared<SweepManager<TKey, TValue>>
      {
         K_FORCE_SHARED(SweepManager)

      public:
         static NTSTATUS Create(
            __in ISweepProvider & sweepProvider,
            __in ConsolidationManager<TKey, TValue>& consolidationManager,
            __in StoreTraceComponent & traceComponent,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(CONSOLIDATIONMANAGER_TAG, allocator) SweepManager(sweepProvider, consolidationManager, traceComponent);

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

         __declspec(property(get = get_NumberOfSweepCycles, put = set_NumberOfSweepCycles)) ULONG32 NumberOfSweepCycles;
         ULONG32 get_NumberOfSweepCycles() const
         {
            return currentSweepCycle_;
         }

         __declspec(property(get = get_MemoryBufferSize, put = set_MemoryBufferSize)) LONG64 MemoryBufferSize;
         LONG64 get_MemoryBufferSize() const
         {
            return memoryBufferSize_;
         }

         void set_MemoryBufferSize(__in LONG64 value)
         {
            memoryBufferSize_ = value;
         }

         // Exposed for testability
         __declspec(property(get = get_TimeoutInMS, put = set_TimeoutInMS)) ULONG TimeoutInMS;
         ULONG get_TimeoutInMS()
         {
             return timeoutInMS_;
         }

         void set_TimeoutInMS(__in ULONG timeout)
         {
             timeoutInMS_ = timeout;
         }

         // Exposed for testability
         __declspec(property(get = get_SweepCompletionSource)) ktl::AwaitableCompletionSource<bool>::SPtr SweepCompletionSource;
         ktl::AwaitableCompletionSource<bool>::SPtr get_SweepCompletionSource()
         {
             return sweepCompletionSourceSPtr_;
         }
         
         ktl::Awaitable<void> SweepAsync()
         {
            ktl::AwaitableCompletionSource<bool>::SPtr sweepCompletionSourceSPtr = nullptr;

            NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(this->GetThisAllocator(), this->GetThisAllocationTag(), sweepCompletionSourceSPtr);
            Diagnostics::Validate(status);

            K_LOCK_BLOCK(closeLock_)
            {
               if (isClosed_)
               {
                  co_return;
               }

               sweepCompletionSourceSPtr_ = sweepCompletionSourceSPtr;
            }

            ktl::CancellationToken cancellationToken = sweepTaskCancellationSourceSPtr_->Token;

            KFinally([&] { sweepCompletionSourceSPtr_->SetResult(true); });

            while (true)
            {
               if (cancellationToken.IsCancellationRequested)
               {
                  break;
               }

               // 1. Create a timer
               KTimer::SPtr localTimerSPtr = nullptr;
               status = KTimer::Create(localTimerSPtr, this->GetThisAllocator(), 'LYSA');
               Diagnostics::Validate(status);

               // Workaround for KTL issue
               timerSPtr_ = localTimerSPtr;

               // 2. Start the timer.
               co_await localTimerSPtr->StartTimerAsync(timeoutInMS_, nullptr);

               // 3. Sweep
               Sweep();
            }
         }

         ktl::Awaitable<void> CancelSweepTaskAsync()
         {
            // If sweep task is in progress, then cancel it
            K_LOCK_BLOCK(closeLock_)
            {
               isClosed_ = true;
               if (sweepTaskCancellationSourceSPtr_ != nullptr)
               {
                  sweepTaskCancellationSourceSPtr_->Cancel();
               }
            }

            if (timerSPtr_ != nullptr)
            {
               timerSPtr_->Cancel();
            }

            if (sweepCompletionSourceSPtr_ != nullptr)
            {
               co_await sweepCompletionSourceSPtr_->GetAwaitable();
            }
         }

         void Sweep()
         {
            LONG64 storeSize = sweepProviderSPtr_->GetMemorySize();
            LONG64 consolidatedStateSize = consolidationManagerSPtr_->GetMemorySize(sweepProviderSPtr_->GetEstimatedKeySize());

            ktl::CancellationToken cancellationToken = sweepTaskCancellationSourceSPtr_->Token;

            // case 1: Current size is lesser than 50% the size of threshold.
            if (storeSize < (memoryBufferSize_ * 0.5))
            {
               // If consolidated state size has increased, then do sweep else skip
               if (consolidatedStateSize > previousConsolidatedStateSize_)
               {
                  SweepUntilMemoryIsBelowThreshold(1, cancellationToken);
               }

               // Don't sweep otherwise
            } // case2: Current size is between 50 to 75% of the the threshold.
            else if (storeSize  < (memoryBufferSize_ * 0.75))
            {
               // If consolidated state size has increased, then do sweep else skip
               if (consolidatedStateSize > previousConsolidatedStateSize_)
               {
                  SweepUntilMemoryIsBelowThreshold(2, cancellationToken);
               }
               else
               {
                  SweepUntilMemoryIsBelowThreshold(1, cancellationToken);
               }
            } // Current size is between than 75% and threshold.
            else if(storeSize  < memoryBufferSize_)
            {
               SweepUntilMemoryIsBelowThreshold(2, cancellationToken);

            } // Current size is greater than memory threhsold, sweep aggresively.
            else
            {
               // Go upto a maximum of 6 cycles and then give up.
               SweepUntilMemoryIsBelowThreshold(6, cancellationToken, memoryBufferSize_);
            }

            previousConsolidatedStateSize_ = consolidatedStateSize;
         }

         void SweepUntilMemoryIsBelowThreshold(
            __in ULONG32 maxSweepCycles,
            __in ktl::CancellationToken const & cancellationToken,
            __in_opt LONG64 memorySize = 0)
         {
            currentSweepCycle_ = 0;

            while (true)
            {
               if (cancellationToken.IsCancellationRequested)
               {
                  break;
               }
               currentSweepCycle_++;

               // Check store size
               LONG64 startSize = sweepProviderSPtr_->GetMemorySize();
               ASSERT_IFNOT(startSize >= 0, "Start size {0} should not be negative", startSize);

               ULONG32 currentSweepCount = consolidationManagerSPtr_->SweepConsolidatedState(cancellationToken);
               
               // Check size
               LONG64 endSize = sweepProviderSPtr_->GetMemorySize();
               ASSERT_IFNOT(endSize >= 0, "End size {0} should not be negative", endSize);


               // Make sure we dont sweep when items have already been swept. If items get loaded soon after the sweepCount check, they will get evicted in the next sweep cycle.
               if (endSize < memorySize || currentSweepCycle_ == maxSweepCycles || currentSweepCycle_ > 2 && currentSweepCount == 0)
               {
                  break;
               }
            }
         }

         private:

            SweepManager(
               __in ISweepProvider & sweepProvider,
               __in ConsolidationManager<TKey, TValue> & consolidationManager,
               __in StoreTraceComponent & traceComponent);

            ULONG timeoutInMS_ = 5 * 60 * 1000;

            LONG64 memoryBufferSize_ = 600 * 1024 * 1024;
            KSharedPtr<ISweepProvider> sweepProviderSPtr_;
            KSharedPtr<ConsolidationManager<TKey, TValue>> consolidationManagerSPtr_;

            StoreTraceComponent::SPtr traceComponent_;
            ULONG32 currentSweepCycle_ = 0;
            ktl::CancellationTokenSource::SPtr sweepTaskCancellationSourceSPtr_ = nullptr;

            ktl::AwaitableCompletionSource<bool>::SPtr sweepCompletionSourceSPtr_ =  nullptr ;
            LONG64 previousConsolidatedStateSize_ = 0;
            KTimer::SPtr timerSPtr_ = nullptr;
            KSpinLock closeLock_;
            bool isClosed_ = false;
         };

         template <typename TKey, typename TValue>
         SweepManager<TKey, TValue>::SweepManager(
            __in ISweepProvider & sweepProvider,
            __in ConsolidationManager<TKey, TValue> & consolidationManager,
            __in StoreTraceComponent & traceComponent) :
            sweepProviderSPtr_(&sweepProvider),
            consolidationManagerSPtr_(&consolidationManager),
            traceComponent_(&traceComponent)
         {
            NTSTATUS status = ktl::CancellationTokenSource::Create(this->GetThisAllocator(), this->GetThisAllocationTag(), sweepTaskCancellationSourceSPtr_);
            if (!NT_SUCCESS(status))
            {
               this->SetConstructorStatus(status);
               return;
            }
         }

         template <typename TKey, typename TValue>
         SweepManager<TKey, TValue>::~SweepManager()
         {
         }
      }
   }
