// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define LOCKHASHVALUE_TAG 'lvHL'

using namespace Data::Utilities;

LockHashValue::LockHashValue()
{
   NTSTATUS status = LockResourceControlBlock::Create(this->GetThisAllocator(), lockResourceControlBlock_);
   this->SetConstructorStatus(status);
}

LockHashValue::~LockHashValue()
{
}

NTSTATUS LockHashValue::Create(
   __in KAllocator& allocator,
   __out LockHashValue::SPtr& result)
{
   result = _new(LOCKHASHVALUE_TAG, allocator) LockHashValue();

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

void LockHashValue::EnterWriteLock()
{
   lock_.Acquire();
}

void LockHashValue::ExitWriteLock()
{
   lock_.Release();
}

void LockHashValue::EnterReadLock()
{
   lock_.Acquire();
}

void LockHashValue::ExitReadLock()
{
   lock_.Release();
}

void LockHashValue::Close()
{

}
