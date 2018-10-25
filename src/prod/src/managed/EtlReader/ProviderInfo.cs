// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Fabric.Strings;
    using System.Runtime.Serialization;

    [DataContract]
    public class ProviderInfo
    {
        [DataMember]
        public const byte DefaultLevel = 0xFF; // Match any level

        [DataMember]
        public const long DefaultKeywords = 0; // Match all events

        public ProviderInfo(Guid id)
            : this(id, ProviderInfo.DefaultKeywords)
        {
        }

        public ProviderInfo(Guid id, long keywords)
            : this(id, keywords, ProviderInfo.DefaultLevel)
        {
        }

        public ProviderInfo(Guid id, long keywords, byte level)
        {
            if (id == Guid.Empty)
            {
                throw new ArgumentException(StringResources.ETLReaderError_EmptyValue, "id");
            }

            this.Id = id;
            this.Keywords = keywords;
            this.Level = level;
        }

        [DataMember]
        public Guid Id
        {
            get;
            private set;
        }

        [DataMember]
        public long Keywords
        {
            get;
            private set;
        }

        [DataMember]
        public byte Level
        {
            get;
            private set;
        }
    }
}