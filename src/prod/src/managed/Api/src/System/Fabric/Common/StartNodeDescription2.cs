// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class StartNodeDescription2
    {
        public StartNodeDescription2(
            StartNodeDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public StartNodeDescriptionKind DescriptionKind
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
            var nativeStartNodeDescription2 = new NativeTypes.FABRIC_START_NODE_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case StartNodeDescriptionKind.UsingNodeName:
                    nativeStartNodeDescription2.Kind = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeStartNodeDescription2.Value =
                        ((StartNodeDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case StartNodeDescriptionKind.UsingReplicaSelector:
                    nativeStartNodeDescription2.Kind = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeStartNodeDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeStartNodeDescription2.Kind = NativeTypes.FABRIC_START_NODE_DESCRIPTION_KIND.FABRIC_START_NODE_DESCRIPTION_KIND_INVALID;
                    nativeStartNodeDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeStartNodeDescription2);
        }

        internal static unsafe StartNodeDescription2 CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_NODE_DESCRIPTION2 native = *(NativeTypes.FABRIC_START_NODE_DESCRIPTION2*)nativeRaw;
            StartNodeDescriptionUsingNodeName startNodeDescriptionUsingNodeName = StartNodeDescriptionUsingNodeName.CreateFromNative(native.Value);

            return new StartNodeDescription2(StartNodeDescriptionKind.UsingNodeName, startNodeDescriptionUsingNodeName);
        }
    }
}
