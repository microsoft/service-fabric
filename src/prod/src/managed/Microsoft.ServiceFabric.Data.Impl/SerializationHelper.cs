// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.IO;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    internal static class SerializationHelper
    {
        public static async Task ReadBytesAsync(ArraySegment<byte> buffer, int count, Stream stream)
        {
            Utility.Assert(buffer.Count >= count, "buffer.Count >= count");
            var totalRead = 0;

            while (totalRead < count)
            {
                var read = await stream.ReadAsync(buffer.Array, buffer.Offset + totalRead, count - totalRead).ConfigureAwait(false);

                if (read == 0)
                {
                    throw new InvalidDataException("Unexpected end of stream.");
                }

                totalRead += read;
            }
        }

        public static async Task<int> ReadIntAsync(ArraySegment<byte> intBuffer, Stream stream)
        {
            await ReadBytesAsync(intBuffer, intBuffer.Count, stream).ConfigureAwait(false);
            return BitConverter.ToInt32(intBuffer.Array, intBuffer.Offset);
        }
    }
}