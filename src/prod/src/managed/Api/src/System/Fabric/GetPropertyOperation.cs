// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a <see cref="System.Fabric.PropertyBatchOperation" /> that gets the specified property if it exists.</para>
    /// </summary>
    /// <remarks>
    /// <para>Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch will fail and not be committed.</para>
    /// </remarks>
    public sealed class GetPropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Instantiates a new instance of the <see cref="System.Fabric.GetPropertyOperation" /> class with specified property name.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <remarks>
        /// <para>
        ///     <see cref="System.Fabric.GetPropertyOperation.IncludeValue" /> is set to <languageKeyword>true</languageKeyword>.</para>
        /// </remarks>
        public GetPropertyOperation(string propertyName)
            : this(propertyName, true)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.GetPropertyOperation" /> with specified property name and include value flag.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="includeValue">
        /// <para>Specifies whether values should be included in the return or only metadata should be returned.</para>
        /// </param>
        public GetPropertyOperation(string propertyName, bool includeValue) 
            : base(propertyName, PropertyBatchOperationKind.Get)
        {
            this.IncludeValue = includeValue;
        }

        internal GetPropertyOperation()
        {
        }

        /// <summary>
        /// <para>Gets a value indicating whether the value of the property is returned together with the metadata.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of the property should be included; <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        public bool IncludeValue { get; internal set; }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            var nativeGetOperation = new NativeTypes.FABRIC_GET_PROPERTY_OPERATION[1];
            nativeGetOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);
            nativeGetOperation[0].IncludeValue = NativeTypes.ToBOOLEAN(this.IncludeValue);
            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET;
            return pin.AddBlittable(nativeGetOperation);
        }
    }
}