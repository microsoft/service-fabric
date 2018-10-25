// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class MoveSecondaryDescription2
    {
        public MoveSecondaryDescription2(
            MoveSecondaryDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public MoveSecondaryDescriptionKind DescriptionKind
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
            var nativeMoveSecondaryDescription2 = new NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case MoveSecondaryDescriptionKind.UsingNodeName:
                    nativeMoveSecondaryDescription2.Kind = NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeMoveSecondaryDescription2.Value =
                        ((MoveSecondaryDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case MoveSecondaryDescriptionKind.UsingReplicaSelector:
                    nativeMoveSecondaryDescription2.Kind = NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeMoveSecondaryDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeMoveSecondaryDescription2.Kind = NativeTypes.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND.FABRIC_MOVE_SECONDARY_DESCRIPTION_KIND_INVALID;
                    nativeMoveSecondaryDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeMoveSecondaryDescription2);
        }
    }
}
