// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class MovePrimaryDescription2
    {
        public MovePrimaryDescription2(
            MovePrimaryDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public MovePrimaryDescriptionKind DescriptionKind
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
            var nativeMovePrimaryDescription2 = new NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case MovePrimaryDescriptionKind.UsingNodeName:
                    nativeMovePrimaryDescription2.Kind = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeMovePrimaryDescription2.Value =
                        ((MovePrimaryDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case MovePrimaryDescriptionKind.UsingReplicaSelector:
                    nativeMovePrimaryDescription2.Kind = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeMovePrimaryDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeMovePrimaryDescription2.Kind = NativeTypes.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND.FABRIC_MOVE_PRIMARY_DESCRIPTION_KIND_INVALID;
                    nativeMovePrimaryDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeMovePrimaryDescription2);
        }
    }
}
