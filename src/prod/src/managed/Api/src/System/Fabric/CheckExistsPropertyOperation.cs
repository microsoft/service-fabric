// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a <see cref="System.Fabric.PropertyBatchOperation" /> that compares the Boolean 
    /// existence of a property with the <see cref="System.Fabric.CheckExistsPropertyOperation.ExistenceCheck" /> argument. </para>
    /// </summary>
    /// <remarks>
    /// <para>The <see cref="System.Fabric.PropertyBatchOperation" /> operation fails if the 
    /// property is not equal to the <see cref="System.Fabric.CheckExistsPropertyOperation.ExistenceCheck" /> argument.
    /// The <see cref="System.Fabric.CheckExistsPropertyOperation" /> is generally used as a precondition for the write operations in the batch. Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch fails and cannot be committed.</para>
    /// </remarks>
    public sealed class CheckExistsPropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Instantiates a <see cref="System.Fabric.CheckExistsPropertyOperation" /> object. </para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>The name of the property.</para>
        /// </param>
        /// <param name="existenceCheck">
        /// <para>Flag that specifies whether the property should exist for the operation to pass.</para>
        /// </param>
        /// <remarks>
        /// <para>If any <see cref="System.Fabric.PropertyBatchOperation" /> in the batch fails,
        /// the entire batch fails and cannot be committed.</para>
        /// </remarks>
        public CheckExistsPropertyOperation(string propertyName, bool existenceCheck)
            : base(propertyName, PropertyBatchOperationKind.CheckExists)
        {
            this.ExistenceCheck = existenceCheck;
        }

        internal CheckExistsPropertyOperation()
        {
        }

        /// <summary>
        /// <para>Gets the flag that specifies whether the property should exist for the operation to pass.</para>
        /// </summary>
        /// <value>
        /// <para>Flag that specifies whether the property should exist for the operation to pass.</para>
        /// </value>
        public bool ExistenceCheck { get; internal set; }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            var nativeCheckExistsOperation = new NativeTypes.FABRIC_CHECK_EXISTS_PROPERTY_OPERATION[1];

            nativeCheckExistsOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);
            nativeCheckExistsOperation[0].ExistenceCheck = NativeTypes.ToBOOLEAN(this.ExistenceCheck);
            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS;
            return pin.AddBlittable(nativeCheckExistsOperation);
        }
    }
}