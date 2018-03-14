// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      class SharedBinaryWriter : 
         public KShared<SharedBinaryWriter>,
         public BinaryWriter
      {
         K_FORCE_SHARED(SharedBinaryWriter)

      public:
         static NTSTATUS
            Create(
               __in KAllocator& Allocator,
               __out SharedBinaryWriter::SPtr& Result);
      };
   }
}

