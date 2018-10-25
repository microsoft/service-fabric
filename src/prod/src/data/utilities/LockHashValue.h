// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace Utilities
   {
      class LockHashValue : public KObject<LockHashValue>, public KShared<LockHashValue>
      {
         K_FORCE_SHARED(LockHashValue)

      public:
         static NTSTATUS
            Create(
               __in KAllocator& allocator,
               __out LockHashValue::SPtr& result);

         __declspec(property(get = get_ResourceControlBlock, put = set_ResourceControlBlock)) LockResourceControlBlock::SPtr ResourceControlBlock;
         LockResourceControlBlock::SPtr get_ResourceControlBlock() const
         {
            return lockResourceControlBlock_;
         }

         void set_ResourceControlBlock(LockResourceControlBlock& value)
         {
            lockResourceControlBlock_ = &value;
         }

         void EnterWriteLock();
         void ExitWriteLock();
         void EnterReadLock();
         void ExitReadLock();
         void Close();

      private:
         LockResourceControlBlock::SPtr lockResourceControlBlock_;
         KSpinLock lock_;
      };
   }
}

