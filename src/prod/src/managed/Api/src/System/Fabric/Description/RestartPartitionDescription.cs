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

    internal sealed class RestartPartitionDescription
    {
        public RestartPartitionDescription(
            Guid operationId,
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode)
        {
            Requires.Argument<Guid>("operationId", operationId).NotNull();
            this.OperationId = operationId;
            this.PartitionSelector = partitionSelector;
            this.RestartPartitionMode = restartPartitionMode;
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

        public RestartPartitionMode RestartPartitionMode
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "OperationId: {0}, PartitionSelector: {1}, RestartPartitionMode: {2}",
                this.OperationId,
                this.PartitionSelector,
                this.RestartPartitionMode);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeRestartPartitionDescription = new NativeTypes.FABRIC_START_PARTITION_RESTART_DESCRIPTION();
            nativeRestartPartitionDescription.OperationId = this.OperationId;

            if (this.PartitionSelector != null)
            {
                nativeRestartPartitionDescription.PartitionSelector = this.PartitionSelector.ToNative(pinCollection);
            }

            nativeRestartPartitionDescription.RestartPartitionMode = (NativeTypes.FABRIC_RESTART_PARTITION_MODE)this.RestartPartitionMode;

            return pinCollection.AddBlittable(nativeRestartPartitionDescription);
        }

        internal static unsafe RestartPartitionDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_PARTITION_RESTART_DESCRIPTION native = *(NativeTypes.FABRIC_START_PARTITION_RESTART_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            NativeTypes.FABRIC_PARTITION_SELECTOR nativePartitionSelector = *(NativeTypes.FABRIC_PARTITION_SELECTOR*)native.PartitionSelector;

            PartitionSelector partitionSelector =
                PartitionSelector.CreateFromNative(nativePartitionSelector);

            return new RestartPartitionDescription(operationId, partitionSelector, (RestartPartitionMode)native.RestartPartitionMode);
        }
    }
}
