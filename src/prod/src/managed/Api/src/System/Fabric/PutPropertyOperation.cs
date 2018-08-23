// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Puts the specified property under the specified name.</para>
    /// </summary>
    /// <remarks>
    /// <para>Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch fails and is not committed.</para>
    /// </remarks>
    public sealed class PutPropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutPropertyOperation" /> class with the specified property name and byte[] value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>A <see cref="System.Byte" /> array that defines the value for the property.</para>
        /// </param>
        public PutPropertyOperation(string propertyName, byte[] value)
            : this(propertyName, PropertyTypeId.Binary, (object)value)
        {
            Requires.Argument<byte[]>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutPropertyOperation" /> class with the specified property name and Int64 value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>An <see cref="System.Int64" /> that defines the value for the property.</para>
        /// </param>
        public PutPropertyOperation(string propertyName, long value)
            : this(propertyName, PropertyTypeId.Int64, (object)value)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutPropertyOperation" /> class with the specified property name and GUID value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>A <see cref="System.Guid" /> that defines the value for the property.</para>
        /// </param>
        public PutPropertyOperation(string propertyName, Guid value)
            : this(propertyName, PropertyTypeId.Guid, (object)value)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutPropertyOperation" /> class with the specified property name and string value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>A <see cref="System.String" /> that defines the value for the property.</para>
        /// </param>
        public PutPropertyOperation(string propertyName, string value)
            : this(propertyName, PropertyTypeId.String, (object)value)
        {
            Requires.Argument<string>("value", value).NotNull();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PutPropertyOperation" /> class with the specified property name and double value.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="value">
        /// <para>A <see cref="System.Double" /> that defines the value for the property.</para>
        /// </param>
        public PutPropertyOperation(string propertyName, double value)
            : this(propertyName, PropertyTypeId.Double, (object)value)
        {
        }

        internal PutPropertyOperation()
        {
        }

        private PutPropertyOperation(string propertyName, PropertyTypeId typeId, object value)
            : base(propertyName, PropertyBatchOperationKind.Put)
        {
            this.PropertyType = typeId;
            this.PropertyValue = value;
        }

        /// <summary>
        /// <para>Gets the property value.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Object" />.</para>
        /// </value>
        public object PropertyValue { get; internal set; }

        /// <summary>
        /// <para>Gets the property type.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.PropertyTypeId" />.</para>
        /// </value>
        public PropertyTypeId PropertyType { get; internal set; }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "Show size of types explicitely in interop code.")]
        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            Requires.Argument<object>("PropertyValue", this.PropertyValue).NotNull();

            var nativePutOperation = new NativeTypes.FABRIC_PUT_PROPERTY_OPERATION[1];
            nativePutOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);

            switch (this.PropertyType)
            {
                case PropertyTypeId.Binary:
                    nativePutOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_BINARY;

                    // Create FABRIC_OPERATION_DATA_BUFFER from the byte[]
                    var valueAsArray = this.PropertyValue as byte[];
                    var nativeOperationBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER[1];
                    nativeOperationBuffer[0].BufferSize = (uint)valueAsArray.Length;
                    nativeOperationBuffer[0].Buffer = pin.AddBlittable(valueAsArray);

                    nativePutOperation[0].PropertyValue = pin.AddBlittable(nativeOperationBuffer);
                    break;
                case PropertyTypeId.Double:
                    nativePutOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_DOUBLE;
                    var valueAsDouble = new Double[1];
                    valueAsDouble[0] = (double)this.PropertyValue;
                    nativePutOperation[0].PropertyValue = pin.AddBlittable(valueAsDouble);
                    break;
                case PropertyTypeId.Guid:
                    nativePutOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_GUID;
                    nativePutOperation[0].PropertyValue = pin.AddBlittable((Guid)this.PropertyValue);
                    break;
                case PropertyTypeId.Int64:
                    nativePutOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INT64;
                    var valueAsLong = new long[1];
                    valueAsLong[0] = (long)this.PropertyValue;
                    nativePutOperation[0].PropertyValue = pin.AddBlittable(valueAsLong);
                    break;
                case PropertyTypeId.String:
                    nativePutOperation[0].PropertyTypeId = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_WSTRING;
                    nativePutOperation[0].PropertyValue = pin.AddBlittable((string)this.PropertyValue);
                    break;
                default:
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeNotSupported_Formatted, this.PropertyType));
            }

            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT;
            return pin.AddBlittable(nativePutOperation);
        }
    }
}