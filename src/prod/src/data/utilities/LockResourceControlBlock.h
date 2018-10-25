// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace Utilities
   {
      class LockResourceControlBlock : public KObject<LockResourceControlBlock>, public KShared<LockResourceControlBlock>
      {
         K_FORCE_SHARED(LockResourceControlBlock)

      public:
         static NTSTATUS
            Create(
               __in KAllocator& allocator,
               __out LockResourceControlBlock::SPtr& result);

         __declspec(property(get = get_LockModeGranted, put = set_LockModeGranted)) LockMode::Enum LockModeGranted;
         LockMode::Enum get_LockModeGranted() const
         {
            return lockModeGranted_;
         }

         void set_LockModeGranted(__in LockMode::Enum value)
         {
            lockModeGranted_ = value;
         }

         __declspec(property(get = get_GrantedList, put = set_GrantedList)) KSharedArray<LockControlBlock::SPtr>::SPtr GrantedList;
         KSharedArray<LockControlBlock::SPtr>::SPtr get_GrantedList() const
         {
            return grantedList_;
         }

         void set_GrantedList(KSharedArray<LockControlBlock::SPtr>& value)
         {
            grantedList_ = &value;
         }

         __declspec(property(get = get_WaitingQueue, put = set_WaitingQueue)) KSharedArray<LockControlBlock::SPtr>::SPtr WaitingQueue;
         KSharedArray<LockControlBlock::SPtr>::SPtr get_WaitingQueue() const
         {
            return waitingQueue_;
         }

         void set_WaitingQueue(__in KSharedArray<LockControlBlock::SPtr>& value)
         {
            waitingQueue_ = &value;
         }

         LockControlBlock::SPtr LocateLockClient(
            __in LONG64 owner,
            __in bool isWaitingList);

         bool IsSingleLockOwnerGranted(__in LONG64 owner);

         bool HasClients();

      private:
         LockResourceControlBlock(
            __in LONG64 owner,
            __in bool isWaitingList);

         LockMode::Enum lockModeGranted_;
         KSharedArray<LockControlBlock::SPtr>::SPtr grantedList_;
         KSharedArray<LockControlBlock::SPtr>::SPtr waitingQueue_;
      };
   }
}
