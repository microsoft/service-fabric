// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    internal sealed class InvokeDataLossDescription
    {
        public InvokeDataLossDescription(
            Guid operationId,
            PartitionSelector partitionSelector,
            DataLossMode dataLossMode)
        {
            Requires.Argument<Guid>("operationId", operationId).NotNull();
            this.OperationId = operationId;
            this.PartitionSelector = partitionSelector;
            this.DataLossMode = dataLossMode;
        }

        public Guid OperationId
        {
            get;
            internal set;
        }

        public PartitionSelector PartitionSelector
        {
            get;
            internal set;
        }

        public DataLossMode DataLossMode
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "OperationId: {0}, PartitionSelector: {1}, DataLossMode: {2}",
                this.OperationId,
                this.PartitionSelector,
                this.DataLossMode);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeInvokeDataLossDescription = new NativeTypes.FABRIC_START_PARTITON_DATA_LOSS_DESCRIPTION();

            nativeInvokeDataLossDescription.OperationId = this.OperationId;

            if (this.PartitionSelector != null)
            {
                nativeInvokeDataLossDescription.PartitionSelector = this.PartitionSelector.ToNative(pinCollection);
            }

            nativeInvokeDataLossDescription.DataLossMode = (NativeTypes.FABRIC_DATA_LOSS_MODE)this.DataLossMode;

            return pinCollection.AddBlittable(nativeInvokeDataLossDescription);
        }

        internal static unsafe InvokeDataLossDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_PARTITON_DATA_LOSS_DESCRIPTION native = *(NativeTypes.FABRIC_START_PARTITON_DATA_LOSS_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            NativeTypes.FABRIC_PARTITION_SELECTOR nativePartitionSelector = *(NativeTypes.FABRIC_PARTITION_SELECTOR*)native.PartitionSelector;

            PartitionSelector partitionSelector =
                PartitionSelector.CreateFromNative(nativePartitionSelector);

            return new InvokeDataLossDescription(operationId, partitionSelector, (DataLossMode)native.DataLossMode);
        }
    }
}
