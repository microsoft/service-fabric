// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::TStore;

NTSTATUS MetadataOperationData::Create(
   __in ULONG32 version,
   __in StoreModificationType::Enum modificationType,
   __in LONG64 transactionId,
   __in_opt OperationData::SPtr operationData,
   __in KAllocator& allocator,
   __out MetadataOperationData::CSPtr & result)
{
   NTSTATUS status;
   CSPtr output = _new(MetadataOperationData_TAG, allocator) MetadataOperationData(version, modificationType, transactionId, operationData);

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

void MetadataOperationData::Serialize()
{
   BinaryWriter writer(GetThisAllocator());
   writer.Write(version_);
   writer.Write(transactionId_);
   writer.Write(static_cast<byte>(modificationType_));
   
   // Write out whether we have valid key bytes.
   bool hasKeyBytes = false;

   if (keyBytesSPtr_ != nullptr)
   {
      hasKeyBytes = true;
   }

   writer.Write(hasKeyBytes);

   Append(*writer.GetBuffer(0));

   if (keyBytesSPtr_ != nullptr)
   {
      Append(*keyBytesSPtr_);
   }
}

MetadataOperationData::CSPtr MetadataOperationData::Deserialize(
   __in ULONG32 version,
   __in KAllocator & allocator,
   __in_opt OperationData::CSPtr operationData)
{
   KInvariant(operationData != nullptr);
   KInvariant(operationData->BufferCount > 0);

   auto metadataSPtr = (*operationData)[0];
   KInvariant(metadataSPtr != nullptr);

   Utilities::BinaryReader reader(*metadataSPtr, allocator);

   // Read version, version starts from 1.
   ULONG32 versionRead = 0;
   reader.Read(versionRead);

   // Check version.
   if (versionRead != version)
   {
      // todo throw unsupported exception
      throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
   }

   // Read transaction id.
   LONG64 transactionId = -1;
   reader.Read(transactionId);

   // Read modification.
   byte modification = 0;
   reader.Read(modification);
   StoreModificationType::Enum modificationType(static_cast<StoreModificationType::Enum>(modification));

   // Read how many value and new value segments there are
   bool hasKeySegment = true;
   reader.Read(hasKeySegment);

   ULONG32 expectedKeySegmentCount = 1;

   if (hasKeySegment)
   {
      expectedKeySegmentCount = 2;
   }

   if (operationData->BufferCount != expectedKeySegmentCount)
   {
      // todo invalid data exception
      throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
   }

   KBuffer::CSPtr readKeyBytesSPtr;
   OperationData::SPtr keyData = nullptr;

   // Read key
   if (hasKeySegment)
   {
      readKeyBytesSPtr = (*operationData)[1];

      keyData = OperationData::Create(allocator);
      keyData->Append(*readKeyBytesSPtr);
   }

   MetadataOperationData::CSPtr metadataOperationData = nullptr;
   NTSTATUS status = MetadataOperationData::Create(version, modificationType, transactionId, keyData, allocator, metadataOperationData);
   Diagnostics::Validate(status);

   return metadataOperationData;
}

//
// Operation data can be null - revisit convention.
//
MetadataOperationData::MetadataOperationData(
   __in ULONG32 version,
   __in StoreModificationType::Enum modificationType,
   __in LONG64 transactionId,
   __in_opt OperationData::SPtr operationData):
   version_(version),
   modificationType_(modificationType),
   transactionId_(transactionId),
   keyBytesSPtr_(operationData)
{
   Serialize();
}

MetadataOperationData::~MetadataOperationData()
{
}
