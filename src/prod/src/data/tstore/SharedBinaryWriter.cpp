// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define SHAREDBINARYWRITER_TAG 'trWS'

using namespace Data::TStore;
using namespace Data::Utilities;

SharedBinaryWriter::SharedBinaryWriter()
   :BinaryWriter(GetThisAllocator())
{
}

SharedBinaryWriter::~SharedBinaryWriter()
{
}

NTSTATUS SharedBinaryWriter::Create(
   __in KAllocator& allocator,
   __out SharedBinaryWriter::SPtr& result)
{
   result = _new(SHAREDBINARYWRITER_TAG, allocator) SharedBinaryWriter();

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
