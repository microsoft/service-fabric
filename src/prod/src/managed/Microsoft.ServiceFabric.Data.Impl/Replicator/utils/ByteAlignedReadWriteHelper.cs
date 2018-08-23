// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Diagnostics;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    internal static class ByteAlignedReadWriteHelper
    {
        public static bool IsAligned(Stream stream, int desiredAlignment = Constants.X64Alignment)
        {
            return GetNumberOfPaddingBytes(stream, desiredAlignment) == 0;
        }

        public static void AssertIfNotAligned(Stream stream, int desiredAlignment = Constants.X64Alignment)
        {
            if (IsAligned(stream, desiredAlignment) == false)
            {
                var message = string.Format(SR.Error_InvalidData_AlignmentExpected, stream.Position);
                Utility.CodingError(message);
            }
        }

        public static void ThrowIfNotAligned(Stream stream, int desiredAlignment = Constants.X64Alignment)
        {
            if (IsAligned(stream, desiredAlignment) == false)
            {
                var message = string.Format(SR.Error_InvalidData_AlignmentExpected, stream.Position);
                throw new InvalidDataException(message);
            }
        }

        public static void WritePaddingUntilAligned(BinaryWriter writer, int desiredAlignment = Constants.X64Alignment)
        {
            var numberOfPaddingBytes = GetNumberOfPaddingBytes(writer.BaseStream, desiredAlignment);
            WriteBadFood(writer, numberOfPaddingBytes);
        }

        public static void ReadPaddingUntilAligned(BinaryReader reader, bool isReserved, int desiredAlignment = Constants.X64Alignment)
        {
            var numberOfPaddingBytes = GetNumberOfPaddingBytes(reader.BaseStream, desiredAlignment);
            ReadBadFood(reader, isReserved, numberOfPaddingBytes);
        }

        public static bool ByteArrayEquals(byte[] original, byte[] other)
        {
            if (original == null || other == null)
            {
                return original == other;
            }

            if (original.Length != other.Length)
            {
                return false;
            }

            for (var i = 0; i < original.Length; i++)
            {
                if (original[i] != other[i])
                {
                    return false;
                }
            }

            return true;
        }

        private static int GetNumberOfPaddingBytes(Stream stream, int desiredAlignment)
        {
            var remainder = stream.Position%desiredAlignment;
            if (remainder == 0)
            {
                return 0;
            }

            var result = unchecked((int) (desiredAlignment - remainder));

            // MCoskun TODO: This can be turned into Test Assert. 
            Utility.Assert(result < desiredAlignment, "{0} < {1}", result, desiredAlignment);

            return result;
        }

        private static void ReadBadFood(BinaryReader reader, bool isReserved, int size)
        {
            // If the padding is reserved, we cannot assert that it is BadFood.
            if (isReserved)
            {
                // Just skip over the bytes.
                reader.BaseStream.Position += size;
                return;
            }

            // We are testing this length because we unrolled bytesBuffer vs BadFood comparison below for perf.
            Debug.Assert(
                Constants.BadFood != null &&
                Constants.BadFood.Length == 4 &&
                Constants.BadFood.Length == Constants.BadFoodLength,
                "BadFood length should be 4.");

            byte[] bytesBuffer = new byte[Constants.BadFoodLength];
            int quotient = size / Constants.BadFoodLength;
            int readLength;

            for (var i = 0; i < quotient; i++)
            {
                readLength = reader.Read(bytesBuffer, 0, Constants.BadFoodLength);

                if (readLength != Constants.BadFoodLength ||
                    Constants.BadFood[0] != bytesBuffer[0] ||
                    Constants.BadFood[1] != bytesBuffer[1] ||
                    Constants.BadFood[2] != bytesBuffer[2] ||
                    Constants.BadFood[3] != bytesBuffer[3])
                {
                    throw new InvalidDataException(SR.Error_InvalidData_PaddingExpected);
                }
            }

            var remainder = size % Constants.BadFoodLength;
            Utility.Assert(remainder < Constants.BadFoodLength, "Remainder {0} < Constants.BadFoodLength", remainder);

            readLength = reader.Read(bytesBuffer, 0, remainder);

            if (readLength != remainder)
            {
                throw new InvalidDataException(SR.Error_InvalidData_PaddingExpected);
            }

            for (int i = 0; i < remainder; i++)
            {
                if (bytesBuffer[i] != Constants.BadFood[i])
                {
                    throw new InvalidDataException(SR.Error_InvalidData_PaddingExpected);
                }
            }
        }

        private static void WriteBadFood(BinaryWriter writer, int size)
        {
            var quotient = size/Constants.BadFoodLength;
            for (var i = 0; i < quotient; i++)
            {
                writer.Write(Constants.BadFood);
            }

            var remainder = size%Constants.BadFoodLength;
            writer.Write(Constants.BadFood, 0, remainder);
        }
    }
}