// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    
    /// <summary>
    /// <para>Represents a <see cref="System.Fabric.PropertyBatchOperation" /> that deletes a specified property if it exists.</para>
    /// </summary>
    /// <remarks>
    /// <para>Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> object fails, the entire batch fails 
    /// and cannot be committed.</para>
    /// </remarks>
    public sealed class DeletePropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Creates and instantiates a <see cref="System.Fabric.DeletePropertyOperation" /> object.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property to be deleted.</para>
        /// </param>
        public DeletePropertyOperation(string propertyName)
            : base(propertyName, PropertyBatchOperationKind.Delete)
        {
        }

        internal DeletePropertyOperation()
        {
        }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            var nativeDeleteOperation = new NativeTypes.FABRIC_DELETE_PROPERTY_OPERATION[1];
            nativeDeleteOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);
            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE;
            return pin.AddBlittable(nativeDeleteOperation);
        }
    }
}