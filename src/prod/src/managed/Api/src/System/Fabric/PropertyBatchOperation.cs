// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the base class for property operations that can be put into a batch and be submitted through the 
    /// <see cref="System.Fabric.FabricClient.PropertyManagementClient.SubmitPropertyBatchAsync(System.Uri,System.Collections.Generic.ICollection{System.Fabric.PropertyBatchOperation},System.TimeSpan,System.Threading.CancellationToken)" /> method.</para>
    /// </summary>
    /// <remarks>
    /// <para>Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> object fails, the entire batch fails and cannot be committed.</para>
    /// </remarks>
    [KnownType(typeof(CheckExistsPropertyOperation))]
    [KnownType(typeof(CheckSequencePropertyOperation))]
    [KnownType(typeof(CheckValuePropertyOperation))]
    [KnownType(typeof(DeletePropertyOperation))]
    [KnownType(typeof(GetPropertyOperation))]
    [KnownType(typeof(PutCustomPropertyOperation))]
    [KnownType(typeof(PutPropertyOperation))]
    public abstract class PropertyBatchOperation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PropertyBatchOperation" /> object.</para>
        /// </summary>
        protected PropertyBatchOperation()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PropertyBatchOperation" /> object with the specified property name and kind.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>String name of the Property.</para>
        /// </param>
        /// <param name="kind">
        /// <para>
        ///     <see cref="System.Fabric.PropertyBatchOperationKind" /> defines the kind of the <see cref="System.Fabric.PropertyBatchOperation" />.</para>
        /// </param>
        protected PropertyBatchOperation(string propertyName, PropertyBatchOperationKind kind)
        {
            Requires.Argument<string>("propertyName", propertyName).NotNullOrEmpty();

            this.PropertyName = propertyName;
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Indicates the name of the property that this <see cref="System.Fabric.PropertyBatchOperation" /> accesses.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string PropertyName { get; internal set; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.PropertyBatchOperationKind" /> that indicates the kind of the <see cref="System.Fabric.PropertyBatchOperation" />.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.PropertyBatchOperationKind" />.</para>
        /// </value>
        /// <remarks>
        /// <para>All Service Fabric enumerations have a reserved "Invalid" field.</para>
        /// </remarks>
        public PropertyBatchOperationKind Kind { get; internal set; }

        internal static unsafe IntPtr ToNative(PinCollection pin, ICollection<PropertyBatchOperation> operations)
        {
            Requires.Argument<ICollection<PropertyBatchOperation>>("operations", operations).NotNull();

            int index = 0;
            NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION[] nativeOperations = new NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION[operations.Count];

            foreach (var operation in operations)
            {
                NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType;
                nativeOperations[index].Value = operation.ToNative(pin, out nativeOperationType);
                nativeOperations[index].Kind = nativeOperationType;
                index++;
            }

            return pin.AddBlittable(nativeOperations);
        }

        internal abstract unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType);
    }
}