// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    internal sealed class MovePrimaryDescriptionUsingNodeName
    {
        public MovePrimaryDescriptionUsingNodeName(
            string newPrimaryNodeName,
            Uri serviceName,
            Guid partitionId,
            bool ignoreConstraints)
        {
            this.NewPrimaryNodeName = newPrimaryNodeName;
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;
            this.IgnoreConstraints = ignoreConstraints;
        }

        public string NewPrimaryNodeName
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
            return string.Format(CultureInfo.InvariantCulture, "NewPrimaryNodeName: {0}, ServiceName: {1}, PartitionId: {2}, IgnoreConstraints: {3}",
                this.NewPrimaryNodeName,
                this.ServiceName,
                this.PartitionId,
                this.IgnoreConstraints);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeMovePrimaryDescriptionUsingNodeName = new NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME();

            nativeMovePrimaryDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NewPrimaryNodeName);

            nativeMovePrimaryDescriptionUsingNodeName.ServiceName = pinCollection.AddObject(this.ServiceName);

            // make it Utility.To...
            nativeMovePrimaryDescriptionUsingNodeName.PartitionId = this.PartitionId;

            nativeMovePrimaryDescriptionUsingNodeName.IgnoreConstraints = NativeTypes.ToBOOLEAN(this.IgnoreConstraints);

            var raw = pinCollection.AddBlittable(nativeMovePrimaryDescriptionUsingNodeName);

            var recreated = CreateFromNative(raw);

            return raw;
        }

        internal static unsafe MovePrimaryDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            var native = *(NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            Uri serviceName = NativeTypes.FromNativeUri(native.ServiceName);

            Guid partitionId = native.PartitionId;

            bool ignoreConstraints = NativeTypes.FromBOOLEAN(native.IgnoreConstraints);

            return new MovePrimaryDescriptionUsingNodeName(nodeName, serviceName, partitionId, ignoreConstraints);
        }
    }
}
