// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace Data::TStore;

NTSTATUS RedoUndoOperationData::Create(
   __in KAllocator & allocator,
   __in_opt OperationData::SPtr valueOperationData,
   __in_opt OperationData::SPtr newValueOperationData,
   __out RedoUndoOperationData::SPtr & result)
{
   result = _new(RedoUndoOperationData_TAG, allocator) RedoUndoOperationData(
      valueOperationData,
      newValueOperationData);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

void RedoUndoOperationData::Serialize()
{
   BinaryWriter writer(GetThisAllocator());
   ULONG32 valueBytesCount = valueOperationDataSPtr_ != nullptr ? valueOperationDataSPtr_->BufferCount : 0;
   writer.Write(valueBytesCount);

   ULONG32 newValueBytesCount = newValueOperationDataSPtr_ != nullptr ? newValueOperationDataSPtr_->BufferCount : 0;
   writer.Write(newValueBytesCount);

   Append(*writer.GetBuffer(0));

   if (valueOperationDataSPtr_ != nullptr)
   {
      KInvariant(valueOperationDataSPtr_->BufferCount == 1);
      Append(*valueOperationDataSPtr_);
   }

   if (newValueOperationDataSPtr_ != nullptr)
   {
      KInvariant(newValueOperationDataSPtr_->BufferCount == 1);
      Append(*newValueOperationDataSPtr_);
   }
}

RedoUndoOperationData::SPtr RedoUndoOperationData::Deserialize(
   __in OperationData const & operationData,
   __in KAllocator& allocator)
{
   // todo: should this be an assert?
   if (operationData.BufferCount == 0)
   {
      throw ktl::Exception(STATUS_INVALID_PARAMETER_1);
   }

   auto metadataSPtr = operationData[0];
   KInvariant(metadataSPtr != nullptr);

   Utilities::BinaryReader reader(*metadataSPtr, allocator);

   // Read number of value and new value segment.
   ULONG32 valueSegmentCount = 0;
   reader.Read(valueSegmentCount);

   ULONG32 newValueSegmentCount = 0;
   reader.Read(newValueSegmentCount);

   // Check the actual number of segments matches the expected number (metadata, key, and count for the value segments)
   ULONG32 expectedSegmentCount = 1 + valueSegmentCount + newValueSegmentCount;

   // todo make it consistent, exception or assert.
   KInvariant(operationData.BufferCount == expectedSegmentCount);

   OperationData::SPtr valueDataSPtr = nullptr;
   OperationData::SPtr newValueDataSPtr = nullptr;

   // Read values.
   if (valueSegmentCount > 0)
   {
      KInvariant(valueSegmentCount == 1);
      valueDataSPtr = OperationData::Create(allocator);
      valueDataSPtr->Append(*operationData[1]);
   }

   if (newValueSegmentCount > 0)
   {
      KInvariant(newValueSegmentCount == 1);
      newValueDataSPtr = OperationData::Create(allocator);
      newValueDataSPtr->Append(*operationData[1 + valueSegmentCount]);
   }

   RedoUndoOperationData::SPtr redoUndoSPtr= nullptr;
   NTSTATUS status = RedoUndoOperationData::Create(allocator, valueDataSPtr, newValueDataSPtr, redoUndoSPtr);
   Diagnostics::Validate(status);

   return redoUndoSPtr;
}        

RedoUndoOperationData::RedoUndoOperationData(
   __in_opt  OperationData::SPtr valueOperationData,
   __in_opt  OperationData::SPtr newValueOperationData)
   :valueOperationDataSPtr_(valueOperationData),
   newValueOperationDataSPtr_(newValueOperationData)
{
   Serialize();
}

RedoUndoOperationData::~RedoUndoOperationData()
{
}

