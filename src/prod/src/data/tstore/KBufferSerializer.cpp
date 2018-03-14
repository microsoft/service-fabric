// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Data::TStore;

   KBufferSerializer::KBufferSerializer()
   {
   }

   KBufferSerializer::~KBufferSerializer()
   {
   }

   NTSTATUS KBufferSerializer::Create(
      __in KAllocator & allocator,
      __out KBufferSerializer::SPtr & result)
   {
      result = _new(BUFFERSERILALIZER_TAG, allocator) KBufferSerializer();

      if (!result)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      return STATUS_SUCCESS;
   }

   void KBufferSerializer::Write(
      __in KBuffer::SPtr value,
      __in BinaryWriter& binaryWriter
   )
   {
      binaryWriter.Write(value.RawPtr());
   }

   KBuffer::SPtr KBufferSerializer::Read(__in BinaryReader& binaryReader)
   {
      KBuffer::SPtr value;
      binaryReader.Read(value);
      return value;
   }

