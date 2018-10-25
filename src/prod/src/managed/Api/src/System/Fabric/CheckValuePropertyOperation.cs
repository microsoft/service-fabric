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
    /// <para>Represents a <see cref="System.Fabric.PropertyBatchOperation" /> that compares the value of the property
    /// with the expected value.  </para>
    /// </summary>
    /// <remarks>
    /// <para>The comparison fails if the value of the property and the expected value are not equal. 
    /// The <see cref="System.Fabric.CheckValuePropertyOperation" /> is generally used as a precondition for the write operations in the batch. 
    /// Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch fails and cannot be committed.</para>
    /// </remarks>
    public sealed class CheckValuePropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.CheckValuePropertyOperation" /> class with specified <paramref name="propertyName" /> and <see cref="System.Byte" />[] value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value of the property.</para>
        /// </param>
        public CheckValuePropertyOperation(string propertyName, byte[] value)
            : this(propertyName, PropertyTypeId.Binary, (object)value)
        {
            Requires.Argument<byte[]>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.CheckValuePropertyOperation" /> class 
        /// with specified <paramref name="propertyName" /> and <see cref="System.Int64" /> value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value of the property.</para>
        /// </param>
        public CheckValuePropertyOperation(string propertyName, long value)
            : this(propertyName, PropertyTypeId.Int64, (object)value)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.CheckValuePropertyOperation" /> class
        /// with specified <paramref name="propertyName" /> and <see cref="System.Guid" /> value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value of the property.</para>
        /// </param>
        public CheckValuePropertyOperation(string propertyName, Guid value)
            : this(propertyName, PropertyTypeId.Guid, (object)value)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.CheckValuePropertyOperation" /> class
        /// with specified <paramref name="propertyName" /> and <see cref="System.String" /> value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value of the property.</para>
        /// </param>
        public CheckValuePropertyOperation(string propertyName, string value)
            : this(propertyName, PropertyTypeId.String, (object)value)
        {
            Requires.Argument<string>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.CheckValuePropertyOperation" /> class
        /// with specified <paramref name="propertyName" /> and <see cref="System.Double" /> value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value of the property.</para>
        /// </param>
        public CheckValuePropertyOperation(string propertyName, double value)
            : this(propertyName, PropertyTypeId.Double, (object)value)
        {
        }

        internal CheckValuePropertyOperation()
        {
        }

        private CheckValuePropertyOperation(string propertyName, PropertyTypeId typeId, object value)
            : base(propertyName, PropertyBatchOperationKind.CheckValue)
        {
            this.PropertyType = typeId;
            this.PropertyValue = value;
        }

        /// <summary>
        /// <para>Gets the value of the property.</para>
        /// </summary>
        /// <value>
        /// <para>The value of the property.</para>
        /// </value>
        public object PropertyValue { get; internal set; }

        /// <summary>
        /// <para>Gets the type of the property.</para>
        /// </summary>
        /// <value>
        /// <para>The type of the property.</para>
        /// </value>
        public PropertyTypeId PropertyType { get; internal set; }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            Requires.Argument<object>("PropertyValue", this.PropertyValue).NotNull();
            
            var nativeCheckValueOperation = new NativeTypes.FABRIC_CHECK_VALUE_PROPERTY_OPERATION[1];
            nativeCheckValueOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);
            
            switch (this.PropertyType)
            {
                case PropertyTypeId.Binary:
                    nativeCheckValueOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_BINARY;
                    
                    // Create FABRIC_OPERATION_DATA_BUFFER from the byte[]
                    var valueAsArray = this.PropertyValue as byte[];
                    var nativeOperationBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER[1];
                    nativeOperationBuffer[0].BufferSize = (uint)valueAsArray.Length;
                    nativeOperationBuffer[0].Buffer = pin.AddBlittable(valueAsArray);

                    nativeCheckValueOperation[0].PropertyValue = pin.AddBlittable(nativeOperationBuffer);
                    break;
                case PropertyTypeId.Double:
                    nativeCheckValueOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_DOUBLE;
                    var valueAsDouble = new double[1];
                    valueAsDouble[0] = (double)this.PropertyValue;
                    nativeCheckValueOperation[0].PropertyValue = pin.AddBlittable(valueAsDouble);
                    break;
                case PropertyTypeId.Guid:
                    nativeCheckValueOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_GUID;
                    nativeCheckValueOperation[0].PropertyValue = pin.AddBlittable((Guid)this.PropertyValue);
                    break;
                case PropertyTypeId.Int64:
                    nativeCheckValueOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INT64;
                    var valueAsLong = new long[1];
                    valueAsLong[0] = (long)this.PropertyValue;
                    nativeCheckValueOperation[0].PropertyValue = pin.AddBlittable(valueAsLong);
                    break;
                case PropertyTypeId.String:
                    nativeCheckValueOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_WSTRING;
                    nativeCheckValueOperation[0].PropertyValue = pin.AddBlittable((string)this.PropertyValue);
                    break;
                default:
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeNotSupported_Formatted, this.PropertyType));
            }

            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE;
            return pin.AddBlittable(nativeCheckValueOperation);
        }
    }   
}