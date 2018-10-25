// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define LOCKHASHTABLE_TAG 'ltHL'

using namespace Data::Utilities;

ULONG MyHashFunction(const ULONG64& Val)
{
   return static_cast<ULONG>(~Val);
}

LockHashTable::LockHashTable()
{
    UnsignedLongComparer::SPtr comparerSPtr;
    NTSTATUS status = UnsignedLongComparer::Create(this->GetThisAllocator(), comparerSPtr);
    this->SetConstructorStatus(status);

    if (!NT_SUCCESS(status))
    {
        return;
    }

    IComparer<ULONG64>::SPtr keyComparerSPtr = static_cast<IComparer<ULONG64> *>(comparerSPtr.RawPtr());
    status = Dictionary<ULONG64, LockHashValue::SPtr>::Create(32, MyHashFunction, *keyComparerSPtr, this->GetThisAllocator(), lockEntriesSPtr_);
    this->SetConstructorStatus(status);
}

LockHashTable::~LockHashTable()
{
}

NTSTATUS LockHashTable::Create(
   __in KAllocator& allocator,
   __out LockHashTable::SPtr& result)
{
   result = _new(LOCKHASHTABLE_TAG, allocator) LockHashTable();

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

void LockHashTable::EnterWriteLock()
{
   lockHashTableLock_.AcquireExclusive();
}

void LockHashTable::ExitWriteLock()
{
   lockHashTableLock_.ReleaseExclusive();
}

void LockHashTable::EnterReadLock()
{
   lockHashTableLock_.AcquireShared();
}

void LockHashTable::ExitReadLock()
{
   lockHashTableLock_.ReleaseShared();
}

void LockHashTable::IncrementEmptyLocksCount()
{
    InterlockedIncrement64(&emptyLocksCount_);
}

void LockHashTable::DecrementEmptyLocksCount()
{
    auto count = InterlockedDecrement64(&emptyLocksCount_);
    ASSERT_IFNOT(count >= 0, "Invalid count={0}", count);
}

void LockHashTable::Close()
{
   lockEntriesSPtr_ = nullptr;
}
