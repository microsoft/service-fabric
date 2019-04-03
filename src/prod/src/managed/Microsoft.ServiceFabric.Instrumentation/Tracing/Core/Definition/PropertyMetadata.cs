// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System.Diagnostics;
    using System.Runtime.Serialization;

    /// <summary>
    /// Describes data about a property.
    /// </summary>
    [DataContract]
    [KnownType(typeof(TraceFieldAttribute))]
    public class PropertyMetadataData
    {
        public PropertyMetadataData(DataType type, string propertyName, TraceFieldAttribute attribute)
        {
            Assert.AssertNotNull(propertyName, "propertyName");
            Assert.AssertNotNull(attribute, "attribute");
            this.Type = type;
            this.Name = propertyName;
            this.AttributeData = attribute;
        }

        [DataMember]
        public DataType Type { get; private set; }

        [DataMember]
        public string Name { get; private set; }

        [DataMember]
        public TraceFieldAttribute AttributeData { get; private set; }
    }
}