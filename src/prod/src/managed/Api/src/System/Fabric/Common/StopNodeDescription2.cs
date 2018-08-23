// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class StopNodeDescription2
    {
        public StopNodeDescription2(
            StopNodeDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public StopNodeDescriptionKind DescriptionKind
        {
            get;
            internal set;
        }

        public object Value
        {
            get;
            internal set;
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStopNodeDescription2 = new NativeTypes.FABRIC_STOP_NODE_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case StopNodeDescriptionKind.UsingNodeName:
                    nativeStopNodeDescription2.Kind = NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_KIND.FABRIC_STOP_NODE_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeStopNodeDescription2.Value =
                        ((StopNodeDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case StopNodeDescriptionKind.UsingReplicaSelector:
                    nativeStopNodeDescription2.Kind = NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_KIND.FABRIC_STOP_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeStopNodeDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeStopNodeDescription2.Kind = NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_KIND.FABRIC_STOP_NODE_DESCRIPTION_KIND_INVALID;
                    nativeStopNodeDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeStopNodeDescription2);
        }

        internal static unsafe StopNodeDescription2 CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_STOP_NODE_DESCRIPTION2 native = *(NativeTypes.FABRIC_STOP_NODE_DESCRIPTION2*)nativeRaw;
            StopNodeDescriptionUsingNodeName stopNodeDescriptionUsingNodeName = StopNodeDescriptionUsingNodeName.CreateFromNative(native.Value);

            return new StopNodeDescription2(StopNodeDescriptionKind.UsingNodeName, stopNodeDescriptionUsingNodeName);
        }
    }
}
