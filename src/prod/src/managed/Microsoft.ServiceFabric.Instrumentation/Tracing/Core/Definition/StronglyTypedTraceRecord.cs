// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;

    /// <summary>
    /// Base class for all strongly typed Trace Records. This class provides helpful functions
    /// that allows the object to be casted as ITraceObject and then the raw fields of Trace can be
    /// accessed. However, this mode is not recommended since it uses reflection to inspect properties, which
    /// can impact the performance.
    /// </summary>
    [DataContract]
    [KnownType(typeof(PropertyMetadataData))]
    public abstract class StronglyTypedTraceRecord : TraceRecord
    {
        protected StronglyTypedTraceRecord(int traceId, TaskName taskName) : base(traceId, taskName)
        {
        }

        private PropertyMetadataData[] metadata;

        /// <inheritdoc />
        public override IEnumerable<PropertyDescriptor> GetPropertyDescriptors()
        {
            if (this.metadata == null)
            {
                this.metadata = ValueReaderCreator.GetPropertyMetadataForType(this.GetType());
            }

            return this.metadata.Select(
                item => new PropertyDescriptor()
                {
                    Index = item.AttributeData.Index,
                    OriginalName = item.AttributeData.OriginalName,
                    FinalType = item.Type,
                    Name = item.Name
                });
        }

        /// <inheritdoc />
        public override object GetPropertyValue(PropertyDescriptor descriptor)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public override object GetPropertyValue(int propertyIndex)
        {
            if (this.metadata == null)
            {
                this.metadata = ValueReaderCreator.GetPropertyMetadataForType(this.GetType());
            }

            if (this.metadata.Length < propertyIndex)
            {
                throw new ArgumentOutOfRangeException(
                    string.Format(CultureInfo.InvariantCulture, "Number of Property: {0}. Trying to access Index: {1}", this.metadata.Length, propertyIndex));
            }

            switch (this.metadata[propertyIndex].Type)
            {
                case DataType.Int32:
                    return this.PropertyValueReader.ReadInt32At(propertyIndex);
                case DataType.Guid:
                    return this.PropertyValueReader.ReadGuidAt(propertyIndex);
                case DataType.Byte:
                    return this.PropertyValueReader.ReadByteAt(propertyIndex);
                case DataType.Ushort:
                    return (ushort)this.PropertyValueReader.ReadInt16At(propertyIndex);
                case DataType.Uint:
                    return (uint)this.PropertyValueReader.ReadInt32At(propertyIndex);
                case DataType.Int64:
                    return this.PropertyValueReader.ReadInt64At(propertyIndex);
                case DataType.Boolean:
                    return this.PropertyValueReader.ReadBoolAt(propertyIndex);
                case DataType.Double:
                    return this.PropertyValueReader.ReadDoubleAt(propertyIndex);
                case DataType.String:
                    return this.PropertyValueReader.ReadUnicodeStringAt(propertyIndex);
                case DataType.DateTime:
                    return this.PropertyValueReader.ReadDateTimeAt(propertyIndex);
                default:
                    return new NotSupportedException(
                        string.Format(CultureInfo.InvariantCulture, "Unknown/Unsupported Data Type: {0}", this.metadata[propertyIndex].Type));
            }
        }

        /// <inheritdoc />
        public override object GetPropertyValue(string propertyName)
        {
            throw new NotImplementedException();
        }

        protected override void InternalOnDeserialized(StreamingContext context)
        {
            this.metadata = null;
        }
    }
}