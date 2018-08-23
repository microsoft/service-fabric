// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{

    /// <summary>
    /// Used to hold record of rawManifest which is inturn a series of events in the eventstream.
    /// </summary>
    internal struct ManifestEnvelope
    {
        internal const int MaxChunkSize = 0xFF00;
        internal enum ManifestFormats : byte
        {
            // Simply dump the XML manifest
            SimpleXmlFormat = 1,
        }

        // If you change these, please update DynamicManifestTraceEventData
        internal ManifestFormats Format;
        internal byte MajorVersion;
        internal byte MinorVersion;
        internal byte Magic;      // This is always 0x5B, which marks this as an envelope (with 1/256 probability of random collision)
        internal ushort TotalChunks;
        internal ushort ChunkNumber;
    };
}