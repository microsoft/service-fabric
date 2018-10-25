// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class RestartNodeDescription2
    {
        public RestartNodeDescription2(
            RestartNodeDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public RestartNodeDescriptionKind DescriptionKind
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
            var nativeRestartNodeDescription2 = new NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case RestartNodeDescriptionKind.UsingNodeName:
                    nativeRestartNodeDescription2.Kind = NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_KIND.FABRIC_RESTART_NODE_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeRestartNodeDescription2.Value =
                        ((RestartNodeDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case RestartNodeDescriptionKind.UsingReplicaSelector:
                    nativeRestartNodeDescription2.Kind = NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_KIND.FABRIC_RESTART_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeRestartNodeDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeRestartNodeDescription2.Kind = NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_KIND.FABRIC_RESTART_NODE_DESCRIPTION_KIND_INVALID;
                    nativeRestartNodeDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeRestartNodeDescription2);
        }
    }
}
