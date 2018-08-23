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

    internal sealed class InvokeQuorumLossDescription
    {
        public InvokeQuorumLossDescription(
            Guid operationId,
            PartitionSelector partitionSelector,
            QuorumLossMode quorumLossMode,
            TimeSpan quorumLossDuration)
        {
            Requires.Argument<Guid>("operationId", operationId).NotNull();
            this.OperationId = operationId;
            this.PartitionSelector = partitionSelector;
            this.QuorumLossMode = quorumLossMode;
            this.QuorumLossDuration = quorumLossDuration;
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

        public QuorumLossMode QuorumLossMode
        {
            get;
            set;
        }

        public TimeSpan QuorumLossDuration
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "OperationId: {0}, PartitionSelector: {1}, QuorumLossMode: {2}, QuorumLossDuration: {3}",
                this.OperationId,
                this.PartitionSelector,
                this.QuorumLossMode,
                this.QuorumLossDuration);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeInvokeQuorumLossDescription = new NativeTypes.FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION();

            nativeInvokeQuorumLossDescription.OperationId = this.OperationId;

            if (this.PartitionSelector != null)
            {
                nativeInvokeQuorumLossDescription.PartitionSelector = this.PartitionSelector.ToNative(pinCollection);
            }

            nativeInvokeQuorumLossDescription.QuorumLossMode = (NativeTypes.FABRIC_QUORUM_LOSS_MODE)this.QuorumLossMode;

            nativeInvokeQuorumLossDescription.QuorumLossDurationInMilliSeconds = Utility.ToMilliseconds(this.QuorumLossDuration, "QuorumLossDuration");

            return pinCollection.AddBlittable(nativeInvokeQuorumLossDescription);
        }

        internal static unsafe InvokeQuorumLossDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION native = *(NativeTypes.FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            NativeTypes.FABRIC_PARTITION_SELECTOR nativePartitionSelector = *(NativeTypes.FABRIC_PARTITION_SELECTOR*)native.PartitionSelector;

            PartitionSelector partitionSelector =
                PartitionSelector.CreateFromNative(nativePartitionSelector);

            TimeSpan quorumLossDuration = TimeSpan.FromMilliseconds(native.QuorumLossDurationInMilliSeconds);

            return new InvokeQuorumLossDescription(operationId, partitionSelector, (QuorumLossMode)native.QuorumLossMode, quorumLossDuration);
        }
    }
}
