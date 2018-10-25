// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the specified property under the specified name and sets the custom type information for custom interpretation of the property value.</para>
    /// </summary>
    /// <remarks>
    /// The custom type is information that is not processed by Service Fabric, 
    /// but can be used by user to serialize/deserialize custom type objects.</remarks>
    public sealed class PutCustomPropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutCustomPropertyOperation" /> class with the specified property name 
        /// and byte[] value, and sets the custom type accordingly.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value for the property.</para>
        /// </param>
        /// <param name="customTypeId">
        /// <para>The user defined custom type.</para>
        /// </param>
        public PutCustomPropertyOperation(string propertyName, byte[] value, string customTypeId)
            : this(propertyName, PropertyTypeId.Binary, (object)value, customTypeId)
        {
            Requires.Argument<byte[]>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutCustomPropertyOperation" /> class with the specified property name 
        /// and Int64 value, and sets the custom type accordingly.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value for the property.</para>
        /// </param>
        /// <param name="customTypeId">
        /// <para>The user defined custom type.</para>
        /// </param>
        public PutCustomPropertyOperation(string propertyName, long value, string customTypeId)
            : this(propertyName, PropertyTypeId.Int64, (object)value, customTypeId)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutCustomPropertyOperation" /> class with the specified property name and GUID 
        /// value, and sets the custom type accordingly.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value for the property.</para>
        /// </param>
        /// <param name="customTypeId">
        /// <para>The user defined custom type.</para>
        /// </param>
        public PutCustomPropertyOperation(string propertyName, Guid value, string customTypeId)
            : this(propertyName, PropertyTypeId.Guid, (object)value, customTypeId)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutCustomPropertyOperation" /> class with the specified property name and 
        /// string value, and sets the custom type accordingly.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value for the property.</para>
        /// </param>
        /// <param name="customTypeId">
        /// <para>The user defined custom type.</para>
        /// </param>
        public PutCustomPropertyOperation(string propertyName, string value, string customTypeId)
            : this(propertyName, PropertyTypeId.String, (object)value, customTypeId)
        {
            Requires.Argument<string>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutCustomPropertyOperation" /> class with the specified property name and 
        /// double value, and sets the custom type accordingly.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value for the property.</para>
        /// </param>
        /// <param name="customTypeId">
        /// <para>The user defined custom type.</para>
        /// </param>
        public PutCustomPropertyOperation(string propertyName, double value, string customTypeId)
            : this(propertyName, PropertyTypeId.Double, (object)value, customTypeId)
        {
        }

        internal PutCustomPropertyOperation()
        {
        }

        private PutCustomPropertyOperation(string propertyName, PropertyTypeId typeId, object value, string customTypeId)
            : base(propertyName, PropertyBatchOperationKind.PutCustom)
        {
            Requires.Argument<string>("customTypeId", customTypeId).NotNullOrEmpty();

            this.PropertyType = typeId;
            this.PropertyValue = value;
            this.CustomTypeId = customTypeId;
        }

        /// <summary>
        /// <para>Gets the property value.</para>
        /// </summary>
        /// <value>
        /// <para>The property value.</para>
        /// </value>
        public object PropertyValue { get; internal set; }

        /// <summary>
        /// <para>Gets the property type.</para>
        /// </summary>
        /// <value>
        /// <para>The property type.</para>
        /// </value>
        public PropertyTypeId PropertyType { get; internal set; }

        /// <summary>
        /// <para>Gets the custom type information. This information can be used by users to serialize/de-serialize custom type objects.</para>
        /// </summary>
        /// <value>
        /// <para>The custom type information.</para>
        /// </value>
        public string CustomTypeId { get; internal set; }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            Requires.Argument<object>("PropertyValue", this.PropertyValue).NotNull();

            var nativePutCustomOperation = new NativeTypes.FABRIC_PUT_CUSTOM_PROPERTY_OPERATION[1];
            nativePutCustomOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);

            switch (this.PropertyType)
            {
                case PropertyTypeId.Binary:
                    nativePutCustomOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_BINARY;

                    // Create FABRIC_OPERATION_DATA_BUFFER from the byte[]
                    var valueAsArray = this.PropertyValue as byte[];
                    var nativeOperationBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER[1];
                    nativeOperationBuffer[0].BufferSize = (uint)valueAsArray.Length;
                    nativeOperationBuffer[0].Buffer = pin.AddBlittable(valueAsArray);

                    nativePutCustomOperation[0].PropertyValue = pin.AddBlittable(nativeOperationBuffer);
                    break;
                case PropertyTypeId.Double:
                    nativePutCustomOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_DOUBLE;
                    var valueAsDouble = new double[1];
                    valueAsDouble[0] = (double)this.PropertyValue;
                    nativePutCustomOperation[0].PropertyValue = pin.AddBlittable(valueAsDouble);
                    break;
                case PropertyTypeId.Guid:
                    nativePutCustomOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_GUID;
                    nativePutCustomOperation[0].PropertyValue = pin.AddBlittable((Guid)this.PropertyValue);
                    break;
                case PropertyTypeId.Int64:
                    nativePutCustomOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INT64;
                    var valueAsLong = new long[1];
                    valueAsLong[0] = (long)this.PropertyValue;
                    nativePutCustomOperation[0].PropertyValue = pin.AddBlittable(valueAsLong);
                    break;
                case PropertyTypeId.String:
                    nativePutCustomOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_WSTRING;
                    nativePutCustomOperation[0].PropertyValue = pin.AddBlittable((string)this.PropertyValue);
                    break;
                default:
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeNotSupported_Formatted, this.PropertyType));
            }

            nativePutCustomOperation[0].PropertyCustomTypeId = pin.AddBlittable(this.CustomTypeId);

            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM;
            return pin.AddBlittable(nativePutCustomOperation);
        }
    }
}