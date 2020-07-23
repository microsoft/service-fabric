// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      namespace StoreModificationType
      {
         enum Enum : byte
         {
            Add = 0,
            Remove = 1,
            Update = 2,
            PartialUpdate = 3,
            Get = 4,
            Clear = 5,
            Checkpoint = 6,
            Copy = 7,
            Pause = 8
         };
      }

      namespace MetadataOperationType
      {
         enum Enum : byte
         {
            Unknown = 0,
            Key = 1,
            KeyAndValue = 2,
         };
      }

      namespace StoreCopyOperation
      {
          enum Enum : byte
          {
              // The operation data contains the version (ULONG32) of the copy protocol
              Version = 0,

              // The operation data contains the entire metadata file
              MetadataTable = 1,

              // The operation data contains the start of a TStore key checkpoint file
              StartKeyFile = 2,

              // The operation data contains a consecutive chunk of a TStore key checkpoint file
              WriteKeyFile = 3,

              // Indicates the end of a TStore key checkpoint file
              EndKeyFile = 4,

              // The operation data contains the start of a TStore value checkpoint file
              StartValueFile = 5,

              // The operation data contains a consecutive chunk of a TStore value checkpoint file
              WriteValueFile = 6,

              // Indicates the end of a TStore value checkpoint file
              EndValueFile = 7,

              // Indicates the copy operation is complete
              Complete = 8
          };
      }

      namespace VolatileStoreCopyOperation
      {
          enum Enum : byte
          {
              // The operation data contains the version (ULONG32) of the copy protocol
              Version = 0,

              // The operation data contains the metadata for the incoming copy
              Metadata = 1,

              // The operation data contains key and value data
              Data = 2,

              // Indicates the copy operation is complete
              Complete = 3
          };
      }

      namespace VolatileStoreCopyOptionalFlags
      {
          enum Enum : byte
          {
              // The operation has a TTL value 
              HasTTL = 1 << 0
          };
      }
   }
}
