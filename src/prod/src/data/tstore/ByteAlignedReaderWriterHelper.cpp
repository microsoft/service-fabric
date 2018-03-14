// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace ktl;
using namespace Data::TStore;
using namespace Data::Utilities;

ByteAlignedReaderWriterHelper::ByteAlignedReaderWriterHelper()
{
}

ByteAlignedReaderWriterHelper::~ByteAlignedReaderWriterHelper()
{
}

bool ByteAlignedReaderWriterHelper::IsAligned(__in ULONG32 position)
{
   return ByteAlignedReaderWriterHelper::GetNumberOfPaddingBytes(position) == 0;
}

void ByteAlignedReaderWriterHelper::AssertIfNotAligned(__in ULONG32 position)
{
    KInvariant(ByteAlignedReaderWriterHelper::IsAligned(position));
}

void ByteAlignedReaderWriterHelper::ThrowIfNotAligned(__in ULONG32 position)
{
   if (IsAligned(ByteAlignedReaderWriterHelper::IsAligned(position)))
   {
      throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
   }
}

void ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(__in BinaryWriter& writer)
{
   ULONG32 numberOfPaddingBytes = GetNumberOfPaddingBytes(writer.Position);
   ULONG32 quotient = static_cast<ULONG32>(numberOfPaddingBytes / Constants::BadFoodLength);
   for (ULONG32 i = 0; i < quotient; i++)
   {
      WriteBadFood(writer, Constants::BadFoodLength);
   }

   ULONG32 remainder = numberOfPaddingBytes % Constants::BadFoodLength;
   WriteBadFood(writer, remainder);
}

void ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(__in BinaryReader& reader)
{
   ULONG32 numberOfPaddingBytes = GetNumberOfPaddingBytes(reader.Position);

   ULONG32 quotient = static_cast<ULONG32>(numberOfPaddingBytes / Constants::BadFoodLength);
   for (ULONG32 i = 0; i < quotient; i++)
   {
      ByteAlignedReaderWriterHelper::ReadBadFood(reader, Constants::BadFoodLength);
   }

   // Read remainder
   ULONG32 remainder = numberOfPaddingBytes % Constants::BadFoodLength;
   KInvariant(remainder < Constants::BadFoodLength);
   ByteAlignedReaderWriterHelper::ReadBadFood(reader, remainder);
}

void ByteAlignedReaderWriterHelper::AppendBadFood(__in BinaryWriter& writer, __in ULONG32 size)
{
   ULONG32 quotient = size / Constants::BadFoodLength;
   for (ULONG32 i = 0; i < quotient; i++)
   {
      WriteBadFood(writer, Constants::BadFoodLength);
   }

   ULONG32 remainder = size%Constants::BadFoodLength;
   WriteBadFood(writer, remainder);
}

ULONG32 ByteAlignedReaderWriterHelper::GetNumberOfPaddingBytes(__in ULONG32 position)
{
   ULONG32 remainder = position%Constants::Alignment;
   if (remainder == 0)
   {
      return 0;
   }

   ULONG32 result = Constants::Alignment - remainder;
   KInvariant(result < Constants::Alignment);
   return result;
}

void ByteAlignedReaderWriterHelper::WriteBadFood(__in BinaryWriter& writer, __in ULONG32 size)
{
   KInvariant(size <= Constants::BadFoodLength);
   for (ULONG32 i = 0; i < size; i++)
   {
      writer.Write(Constants::BadFood[i]);
   }
}

void ByteAlignedReaderWriterHelper::ReadBadFood(__in BinaryReader& reader, __in ULONG32 size)
{
   KInvariant(size <= Constants::BadFoodLength);
   for (ULONG32 i = 0; i < size; i++)
   {
      byte byte;
      reader.Read(byte);
      KInvariant(byte == Constants::BadFood[i]);
   }
}

