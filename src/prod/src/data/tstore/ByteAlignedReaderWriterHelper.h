// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      class ByteAlignedReaderWriterHelper
      {
      public:
         ByteAlignedReaderWriterHelper();
         ~ByteAlignedReaderWriterHelper();

         static bool IsAligned(__in ULONG32 position);
         static void AssertIfNotAligned(__in ULONG32 position);
         static void ThrowIfNotAligned(__in ULONG32 position);
         static void WritePaddingUntilAligned(__in BinaryWriter& writer);
         static void ReadPaddingUntilAligned(__in BinaryReader& reader);
         static void AppendBadFood(__in BinaryWriter& writer, __in ULONG32 size);

      private:
         static void WriteBadFood(__in BinaryWriter& writer, __in ULONG32 size);
         static void ReadBadFood(__in BinaryReader& reader, __in ULONG32 size);
         static ULONG32 GetNumberOfPaddingBytes(__in ULONG32 position);
      };
   }
}
