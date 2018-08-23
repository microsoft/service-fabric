// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

    [DataContract]
    [StructLayout(LayoutKind.Sequential)]
    internal struct TraceEventInfo
    {
        [DataMember]
        public Guid ProviderGuid;

        [DataMember]
        public Guid EventGuid;

        [DataMember]
        public EventDescriptor EventDescriptor;

        [DataMember]
        public DecodingSource DecodingSource;

        [DataMember]
        public uint ProviderNameOffset;

        [DataMember]
        public uint LevelNameOffset;

        [DataMember]
        public uint ChannelNameOffset;

        [DataMember]
        public uint KeywordsNameOffset;

        [DataMember]
        public uint TaskNameOffset;

        [DataMember]
        public uint OpcodeNameOffset;

        [DataMember]
        public uint EventMessageOffset;

        [DataMember]
        public uint ProviderMessageOffset;

        [DataMember]
        public uint BinaryXmlOffset;

        [DataMember]
        public uint BinaryXmlSize;

        [DataMember]
        public uint ActivityIDNameOffset;

        [DataMember]
        public uint RelatedActivityIDNameOffset;

        [DataMember]
        public uint PropertyCount;

        [DataMember]
        public uint TopLevelPropertyCount;

        [DataMember]
        public TemplateFlags Flags;
    }
}