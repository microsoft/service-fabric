// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;

    internal sealed class RestartDeployedCodePackageDescription2
    {
        public RestartDeployedCodePackageDescription2(
            RestartDeployedCodePackageDescriptionKind descriptionKind,
            object value)
        {
            this.DescriptionKind = descriptionKind;
            this.Value = value;
        }

        public RestartDeployedCodePackageDescriptionKind DescriptionKind
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
            var nativeRestartDeployedCodePackageDescription2 = new NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION2();

            switch (this.DescriptionKind)
            {
                case RestartDeployedCodePackageDescriptionKind.UsingNodeName:
                    nativeRestartDeployedCodePackageDescription2.Kind = NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND_USING_NODE_NAME;
                    nativeRestartDeployedCodePackageDescription2.Value =
                        ((RestartDeployedCodePackageDescriptionUsingNodeName)this.Value).ToNative(pinCollection);
                    break;
                case RestartDeployedCodePackageDescriptionKind.UsingReplicaSelector:
                    nativeRestartDeployedCodePackageDescription2.Kind = NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
                    // not implemented now
                    nativeRestartDeployedCodePackageDescription2.Value = IntPtr.Zero;
                    break;
                default:
                    nativeRestartDeployedCodePackageDescription2.Kind = NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_KIND_INVALID;
                    nativeRestartDeployedCodePackageDescription2.Value = IntPtr.Zero;
                    break;
            }

            return pinCollection.AddBlittable(nativeRestartDeployedCodePackageDescription2);
        }
    }
}
