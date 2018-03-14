// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define BUFFERSERILALIZER_TAG 'lsBK'

namespace Data
{
   namespace TStore
   {
      class KBufferSerializer :
         public KObject<KBufferSerializer>
         , public KShared<KBufferSerializer>
         , public Data::StateManager::IStateSerializer<KBuffer::SPtr>
      {
         K_FORCE_SHARED(KBufferSerializer)
         K_SHARED_INTERFACE_IMP(IStateSerializer)
      public:
         static NTSTATUS Create(
            __in KAllocator & allocator,
            __out KBufferSerializer::SPtr & result);

         void Write(
            __in KBuffer::SPtr value,
            __in Data::Utilities::BinaryWriter& binaryWriter);

         KBuffer::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
      };
   }
}
