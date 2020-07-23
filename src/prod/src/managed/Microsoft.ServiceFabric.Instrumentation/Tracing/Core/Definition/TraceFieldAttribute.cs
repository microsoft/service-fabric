// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Defines the attributes for fields in a trace record.
    /// </summary>
    [DataContract]
    public sealed class TraceFieldAttribute : Attribute
    {
        public TraceFieldAttribute(int index)
        {
            if (index < 0)
            {
                throw new ArgumentException("index");
            }

            this.Index = index;
            this.OriginalName = null;
            this.Version = null;
            this.DefaultValue = string.Empty;
        }

        public TraceFieldAttribute(int index, int version)
        {
            if (index < 0)
            {
                throw new ArgumentException("index");
            }

            this.Index = index;
            this.OriginalName = null;
            this.Version = version;
            this.DefaultValue = string.Empty;
        }

        [DataMember]
        public int Index { get; private set; }

        [DataMember]
        public int? Version { get; private set; }

        [DataMember]
        public string OriginalName { get; set; }

        [DataMember]
        public bool Optional { get; set; }

        [DataMember]
        public object DefaultValue { get; set; }
    }
}