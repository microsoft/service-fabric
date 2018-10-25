// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;

    internal class PartialManifestInfo
    {
        private const int MagicNumber = 0x5B;
        private const int HeaderSizeInBytes = 8;
        private byte[][] Chunks;
        private int ChunksLeft;

        private byte majorVersion;
        private byte minorVersion;
        private ManifestEnvelope.ManifestFormats format;

        internal unsafe byte[] AddChunk(byte[] data)
        {

            int dataLen = data.Length;

            if (dataLen <= sizeof (ManifestEnvelope) || data[3] != MagicNumber) // magic number 
                goto Fail;

            ushort totalChunks = BitConverter.ToUInt16(data, 4);
            ushort chunkNum = BitConverter.ToUInt16(data, 6);
            if (chunkNum >= totalChunks || totalChunks == 0)
                goto Fail;

            if (Chunks == null)
            {
                // To allow for resyncing at 0, otherwise we fail aggressively. 
                if (chunkNum != 0)
                    goto Fail;

                format = (ManifestEnvelope.ManifestFormats) data[0];
                majorVersion = (byte) data[1];
                minorVersion = (byte) data[2];
                ChunksLeft = totalChunks;
                Chunks = new byte[ChunksLeft][];
            }
            else
            {
                // Chunks have to agree with the format and version information. 
                if (format != (ManifestEnvelope.ManifestFormats) data[0] ||
                    majorVersion != data[1] || minorVersion != data[2])
                    goto Fail;
            }

            if (Chunks.Length <= chunkNum || Chunks[chunkNum] != null)
                goto Fail;

            byte[] chunk = new byte[dataLen - HeaderSizeInBytes];
            Buffer.BlockCopy(data, HeaderSizeInBytes, chunk, 0, chunk.Length);
            Chunks[chunkNum] = chunk;
            --ChunksLeft;
            if (ChunksLeft > 0)
                return null;

            // OK we have a complete set of chunks
            byte[] serializedData = null;
            if (Chunks.Length > 1)
            {
                int totalLength = 0;
                for (int i = 0; i < Chunks.Length; i++)
                    totalLength += Chunks[i].Length;

                // Concatenate all the arrays. 
                serializedData = new byte[totalLength];
                int pos = 0;
                for (int i = 0; i < Chunks.Length; i++)
                {
                    Array.Copy(Chunks[i], 0, serializedData, pos, Chunks[i].Length);
                    pos += Chunks[i].Length;
                }
            }
            else
            {
                serializedData = Chunks[0];
            }

            Chunks = null;
            return serializedData;

            Fail:
            Chunks = null;
            return null;
        }
    }
}