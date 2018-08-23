// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define LOCKRESOURCECONTROLBLOCK_TAG 'klBL'

using namespace Data::Utilities;

LockResourceControlBlock::LockResourceControlBlock()
   : grantedList_(nullptr),
   waitingQueue_(nullptr)
{
   grantedList_ =  _new(LOCKRESOURCECONTROLBLOCK_TAG, GetThisAllocator()) KSharedArray<KSharedPtr<LockControlBlock>>();
   waitingQueue_ = _new(LOCKRESOURCECONTROLBLOCK_TAG, GetThisAllocator()) KSharedArray<KSharedPtr<LockControlBlock>>();
}

LockResourceControlBlock:: ~LockResourceControlBlock()
{
}
 
NTSTATUS LockResourceControlBlock::Create(
   __in KAllocator& allocator,
   __out LockResourceControlBlock::SPtr& result)
{
   result = _new(LOCKRESOURCECONTROLBLOCK_TAG, allocator) LockResourceControlBlock();

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

LockControlBlock::SPtr LockResourceControlBlock::LocateLockClient(
   __in LONG64 owner,
   __in bool isWaitingList)
{
   LockControlBlock::SPtr localControlBlockSPtr = nullptr;

   KSharedArray<LockControlBlock::SPtr>::SPtr workingList = nullptr;
   if (isWaitingList)
   {
      workingList = waitingQueue_;
   }
   else
   {
      workingList = grantedList_;
   }

   for (ULONG32 index = 0; index < workingList->Count(); index++)
   {
      if (owner == (*workingList)[index]->GetOwner())
      {
         localControlBlockSPtr = (*workingList)[index];
         break;
      }
   }

   return localControlBlockSPtr;
}

bool LockResourceControlBlock::IsSingleLockOwnerGranted(__in LONG64 owner)
{
   if (owner < 0)
   {
      if (GrantedList->Count() != 0)
      {
         owner = (*GrantedList)[0]->GetOwner();
      }
      else
      {
         return false;
      }
   }

   for (ULONG32 index = 0; index < GrantedList->Count(); index++)
   {
      if ((*GrantedList)[index]->GetOwner() != owner)
      {
         return false;
      }
   }

   return true;
}

bool LockResourceControlBlock::HasClients()
{
    return (GrantedList->Count() + WaitingQueue->Count()) != 0;
}
