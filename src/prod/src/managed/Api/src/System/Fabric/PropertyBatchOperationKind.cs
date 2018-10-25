// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the kind of the BatchPropertyOperation.</para>
    /// </summary>
    public enum PropertyBatchOperationKind
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have a reserved “Invalid” flag.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.PutPropertyOperation" />.</para>
        /// </summary>
        Put = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.GetPropertyOperation" />.</para>
        /// </summary>
        Get = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.CheckExistsPropertyOperation" />.</para>
        /// </summary>
        CheckExists = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.CheckSequencePropertyOperation" />.</para>
        /// </summary>
        CheckSequence = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.DeletePropertyOperation" />.</para>
        /// </summary>
        Delete = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.PutCustomPropertyOperation" />.</para>
        /// </summary>
        PutCustom = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.PropertyBatchOperation" /> is of type <see cref="System.Fabric.CheckValuePropertyOperation" />.</para>
        /// </summary>
        CheckValue = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE,
    }
}