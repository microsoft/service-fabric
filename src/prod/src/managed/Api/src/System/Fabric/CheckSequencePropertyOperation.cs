// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Compares the <see cref="System.Fabric.NamedPropertyMetadata.SequenceNumber" /> of a property
    /// with the <see cref="System.Fabric.CheckSequencePropertyOperation.SequenceNumber" /> argument. </para>
    /// </summary>
    /// <remarks>
    /// <para>The comparison fails if the sequence numbers are not equal. 
    /// <see cref="System.Fabric.CheckSequencePropertyOperation" /> is generally used as a precondition for the write operations in the batch. 
    /// Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch fails and cannot be committed.</para>
    /// </remarks>
    public sealed class CheckSequencePropertyOperation : PropertyBatchOperation
    {
        /// <summary>
        /// <para>Instantiates a new instance of the <see cref="System.Fabric.CheckSequencePropertyOperation" /> class.</para>
        /// </summary>
        /// <param name="propertyName">
        /// <para>A <see cref="System.String" /> that defines the name of the property.</para>
        /// </param>
        /// <param name="sequenceNumber">
        /// <para>A <see cref="System.Int64" /> that defines the expected sequence number of the property for the operation to pass.</para>
        /// </param>
        /// <remarks>
        /// <para>Note that if one <see cref="System.Fabric.PropertyBatchOperation" /> fails, the entire batch fails and cannot be committed.</para>
        /// </remarks>
        public CheckSequencePropertyOperation(string propertyName, long sequenceNumber)
            : base(propertyName, PropertyBatchOperationKind.CheckSequence)
        {
            this.SequenceNumber = sequenceNumber;
        }

        internal CheckSequencePropertyOperation()
        {
        }

        /// <summary>
        /// <para>Gets the expected sequence number.</para>
        /// </summary>
        /// <value>
        /// <para>The expected sequence number.</para>
        /// </value>
        public long SequenceNumber { get; internal set; }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND nativeOperationType)
        {
            var nativeCheckSequenceOperation = new NativeTypes.FABRIC_CHECK_SEQUENCE_PROPERTY_OPERATION[1];

            nativeCheckSequenceOperation[0].PropertyName = pin.AddBlittable(this.PropertyName);
            nativeCheckSequenceOperation[0].SequenceNumber = this.SequenceNumber;

            nativeOperationType = NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND.FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE;
            return pin.AddBlittable(nativeCheckSequenceOperation);
        }
    }
}