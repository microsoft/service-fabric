// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace Utilities
   {
      class LockManager;
      class LockControlBlock : public KObject<LockControlBlock>, public KShared<LockControlBlock> //public ILock
      {
         K_FORCE_SHARED(LockControlBlock)
      public:
         static NTSTATUS
            Create(
               __in LockManager& lockManager,
               __in LONG64 owner,
               __in ULONG64 resourceNameHash,
               __in LockMode::Enum mode,
               __in Common::TimeSpan timeout,
               __in LockStatus::Enum status,
               __in bool isUpgraded,
               __in KAllocator& Allocator,
               __out LockControlBlock::SPtr& Result);

         __declspec(property(get = get_LockCount, put = set_LockCount)) ULONG32 LockCount;
         ULONG32 get_LockCount() const
         {
            return count_;
         }

         void set_LockCount(__in ULONG32 value)
         {
            count_ = value;
         }

         __declspec(property(get = get_Waiter, put = set_Waiter)) KSharedPtr<ktl::AwaitableCompletionSource<KSharedPtr<LockControlBlock>>> Waiter;
         KSharedPtr<ktl::AwaitableCompletionSource<KSharedPtr<LockControlBlock>>> get_Waiter() const
         {
            return waiterSPtr_;
         }

         void set_Waiter(__in ktl::AwaitableCompletionSource<KSharedPtr<LockControlBlock>>& value)
         {
            waiterSPtr_ = &value;
         }

         __declspec(property(get = get_LockResourceNameHash, put = set_LockResourceNameHash)) ULONG64 LockResourceNameHash;
         ULONG64 get_LockResourceNameHash() const
         {
            return lockResourceNameHash_;
         }

         void set_LockResourceNameHash(__in ULONG64 value)
         {
            lockResourceNameHash_ = value;
         }

         LONG64 GetOwner() const /*override*/;
         LockMode::Enum GetLockMode() const /*override*/;
         Common::TimeSpan GetTimeOut() const /*override*/;
         LockStatus::Enum GetStatus() const /*override*/;
         LONG64 GetGrantedTime() const /*override*/;
         ULONG32 GetCount() const /*override*/;
         bool IsUgraded() const /*override*/;
         KSharedPtr<LockManager> GetLockManager() const /*override*/;

         void SetGrantedTime(__in LONG64 value);
         void SetStatus(__in LockStatus::Enum lockStatus);

         void StartExpire();
         bool StopExpire();
         void Close();

      private:
         LockControlBlock(
            __in LockManager& lockManager,
            __in LONG64 owner,
            __in ULONG64 resourceNameHash,
            __in LockMode::Enum mode,
            __in Common::TimeSpan timeout,
            __in LockStatus::Enum status,
            __in bool isUpgraded);

         void Expire(
            __in KAsyncContextBase* const,
            __in KAsyncContextBase&);

         KSharedPtr<LockManager> lockManagerSPtr_;
         LONG64 lockOwner_;
         ULONG64 lockResourceNameHash_;
         LockMode::Enum lockMode_;
         Common::TimeSpan timeOut_;
         LockStatus::Enum lockStatus_;
         ULONG32 lockCount_;
         ULONG64 grantTime_;
         KSharedPtr<ktl::AwaitableCompletionSource<KSharedPtr<LockControlBlock>>> waiterSPtr_;
         bool isUpgradedLock_;
         ULONG32 count_;
         KTimer::SPtr timerSPtr_;
      };
   }
}
