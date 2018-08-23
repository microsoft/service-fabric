// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Numerics;

    internal sealed class RestartNodeDescriptionUsingNodeName
    {
        public RestartNodeDescriptionUsingNodeName(
            string nodeName,
            BigInteger nodeInstance,
            bool shouldCreateFabricDump,
            CompletionMode completionMode)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.ShouldCreateFabricDump = shouldCreateFabricDump;
            this.CompletionMode = completionMode;
        }

        public string NodeName
        {
            get;
            internal set;
        }

        public BigInteger NodeInstance
        {
            get;
            internal set;
        }

        public bool ShouldCreateFabricDump
        {
            get; 
            internal set;
        }

        public CompletionMode CompletionMode
        {
            get; 
            internal set; 
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NodeName: {0}, NodeInstance: {1}, ShouldCreateFabricDump: {2}, Completion mode: {3}",
                this.NodeName,
                this.NodeInstance.ToString(),
                this.ShouldCreateFabricDump,
                this.CompletionMode);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeRestartNodeDescriptionUsingNodeName = new NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_USING_NODE_NAME();

            nativeRestartNodeDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NodeName);

            // make it Utility.To...
            nativeRestartNodeDescriptionUsingNodeName.NodeInstance = (ulong)this.NodeInstance;

            nativeRestartNodeDescriptionUsingNodeName.ShouldCreateFabricDump =
                NativeTypes.ToBOOLEAN(this.ShouldCreateFabricDump);

            switch (this.CompletionMode)
            {
                case CompletionMode.DoNotVerify:
                    nativeRestartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_DO_NOT_VERIFY;
                    break;
                case CompletionMode.Verify:
                    nativeRestartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_VERIFY;
                    break;
                default:
                    nativeRestartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_INVALID;
                    break;
            }

            return pinCollection.AddBlittable(nativeRestartNodeDescriptionUsingNodeName);
        }

        internal static unsafe RestartNodeDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_USING_NODE_NAME native = *(NativeTypes.FABRIC_RESTART_NODE_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            BigInteger nodeInstance = new BigInteger(native.NodeInstance);

            bool shouldCreateFabricDump = NativeTypes.FromBOOLEAN(native.ShouldCreateFabricDump);

            CompletionMode completionMode = CompletionMode.Invalid;

            switch (native.CompletionMode)
            {
                case NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_DO_NOT_VERIFY:
                    completionMode = CompletionMode.DoNotVerify;
                    break;
                case NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_VERIFY:
                    completionMode = CompletionMode.Verify;
                    break;
                default:
                    completionMode = CompletionMode.Invalid;
                    break;
            }

            return new RestartNodeDescriptionUsingNodeName(nodeName, nodeInstance, shouldCreateFabricDump, completionMode);
        }
    }
}
