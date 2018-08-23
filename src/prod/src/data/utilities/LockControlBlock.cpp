// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define LOCKCONTROLBLOCK_TAG 'klBL'

using namespace Data::Utilities;

LockControlBlock::LockControlBlock(
   __in LockManager& lockManager,
   __in LONG64 owner,
   __in ULONG64 resourceNameHash,
   __in LockMode::Enum mode,
   __in Common::TimeSpan timeout,
   __in LockStatus::Enum status,
   __in bool isUpgraded)
: lockManagerSPtr_(&lockManager),
lockOwner_(owner),
lockResourceNameHash_(resourceNameHash),
lockMode_(mode),
timeOut_(timeout),
lockStatus_(status),
waiterSPtr_(nullptr),
isUpgradedLock_(isUpgraded),
count_(1),
timerSPtr_(nullptr)
{
}

LockControlBlock::~LockControlBlock()
{
}

NTSTATUS LockControlBlock::Create(
   __in LockManager& lockManager,
   __in LONG64 owner,
   __in ULONG64 resourceNameHash,
   __in LockMode::Enum mode,
   __in  Common::TimeSpan timeout,
   __in LockStatus::Enum status,
   __in bool isUpgraded,
   __in KAllocator& allocator,
   __out LockControlBlock::SPtr& result)
{
   result = _new(LOCKCONTROLBLOCK_TAG, allocator) LockControlBlock(
      //traceType,
      lockManager,
      owner,
      resourceNameHash,
      mode,
      timeout,
      status,
      isUpgraded);

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

LONG64 LockControlBlock::GetOwner() const 
{
   return lockOwner_;
}

LockMode::Enum LockControlBlock::GetLockMode() const
{
   return lockMode_;
}

Common::TimeSpan LockControlBlock::GetTimeOut() const
{
   return timeOut_;
}

LockStatus::Enum LockControlBlock::GetStatus() const
{
   return lockStatus_;
}

void LockControlBlock::SetStatus(__in LockStatus::Enum lockStatus)
{
   lockStatus_ = lockStatus;
}

LONG64 LockControlBlock::GetGrantedTime() const
{
   return grantTime_;
}

ULONG32 LockControlBlock::GetCount() const
{
   return count_;
}

bool LockControlBlock::IsUgraded() const
{
   return isUpgradedLock_;
}

KSharedPtr<LockManager> LockControlBlock::GetLockManager() const
{
   return lockManagerSPtr_;
}

void LockControlBlock::SetGrantedTime(__in LONG64 value)
{
   grantTime_ = value;
}

void LockControlBlock::StartExpire()
{
    if (timeOut_ != Common::TimeSpan::MaxValue)
    {
        NTSTATUS status = KTimer::Create(timerSPtr_, GetThisAllocator(), LOCKCONTROLBLOCK_TAG);
        THROW_ON_FAILURE(status);

        KAsyncContextBase::CompletionCallback callback;
        callback.Bind(this, &LockControlBlock::Expire);
        this->AddRef();
        timerSPtr_->StartTimer(static_cast<ULONG>(timeOut_.TotalMilliseconds()), nullptr, callback);
    }
}

bool LockControlBlock::StopExpire()
{
   if (timerSPtr_ != nullptr)
   {
      BOOLEAN success = timerSPtr_->Cancel();
      if (success == TRUE)
      {
         return true;
      }

      return false;
   }

   return true;
}

void LockControlBlock::Close()
{
    waiterSPtr_ = nullptr;
}

void LockControlBlock::Expire(
   __in KAsyncContextBase* const,
   __in KAsyncContextBase& context)
{
    // Check only on debug builds.
    KAssert(&context == timerSPtr_.RawPtr());
    if (context.Status() != STATUS_CANCELLED)
    {
        ASSERT_IFNOT(lockManagerSPtr_ != nullptr, "lockManagerSPtr_ != nullptr");
        lockManagerSPtr_->ExpireLock(*this);
    }

   this->Release();
}



