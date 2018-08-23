// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    internal sealed class MoveSecondaryDescriptionUsingNodeName
    {
        public MoveSecondaryDescriptionUsingNodeName(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            Uri serviceName,
            Guid partitionId,
            bool ignoreConstraints)
        {
            this.CurrentSecondaryNodeName = currentSecondaryNodeName;
            this.NewSecondaryNodeName = newSecondaryNodeName;
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;
            this.IgnoreConstraints = ignoreConstraints;
        }

        public string CurrentSecondaryNodeName
        {
            get;
            internal set;
        }

        public string NewSecondaryNodeName
        {
            get;
            internal set;
        }

        public Uri ServiceName
        {
            get;
            internal set;
        }

        public Guid PartitionId
        {
            get; 
            internal set; 
        }

        public bool IgnoreConstraints
        {
            get; 
            internal set; 
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Current NodeName: {0}, New NodeName: {1}, ServiceName: {2}, PartitionId: {3}, IgnoreConstraints: {4}",
                this.CurrentSecondaryNodeName,
                this.NewSecondaryNodeName,
                this.ServiceName,
                this.PartitionId,
                this.IgnoreConstraints);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeMoveSecondaryDescriptionUsingNodeName = new NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME();

            nativeMoveSecondaryDescriptionUsingNodeName.CurrentNodeName = pinCollection.AddObject(this.CurrentSecondaryNodeName);

            nativeMoveSecondaryDescriptionUsingNodeName.NewNodeName = pinCollection.AddObject(this.NewSecondaryNodeName);

            nativeMoveSecondaryDescriptionUsingNodeName.ServiceName = pinCollection.AddObject(this.ServiceName);

            nativeMoveSecondaryDescriptionUsingNodeName.PartitionId = this.PartitionId;

            nativeMoveSecondaryDescriptionUsingNodeName.IgnoreConstraints = NativeTypes.ToBOOLEAN(this.IgnoreConstraints);

            var raw = pinCollection.AddBlittable(nativeMoveSecondaryDescriptionUsingNodeName);

            var recreated = CreateFromNative(raw);

            return raw;
        }

        internal static unsafe MoveSecondaryDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            var native = *(NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string currentNodeName = NativeTypes.FromNativeString(native.CurrentNodeName);
            string newNodeName = NativeTypes.FromNativeString(native.NewNodeName);

            Uri serviceName = NativeTypes.FromNativeUri(native.ServiceName);

            Guid partitionId = native.PartitionId;

            bool ignoreConstraints = NativeTypes.FromBOOLEAN(native.IgnoreConstraints);

            return new MoveSecondaryDescriptionUsingNodeName(currentNodeName, newNodeName, serviceName, partitionId, ignoreConstraints);
        }
    }
}
