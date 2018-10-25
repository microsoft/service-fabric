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

    internal sealed class StopNodeDescriptionUsingNodeName
    {
        public StopNodeDescriptionUsingNodeName(
            string nodeName,
            BigInteger nodeInstance,
            CompletionMode completionMode)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
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

        public CompletionMode CompletionMode
        {
            get; 
            internal set; 
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NodeName: {0}, NodeInstance: {1}, Completion mode: {2}",
                this.NodeName,
                this.NodeInstance.ToString(),
                this.CompletionMode);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStopNodeDescriptionUsingNodeName = new NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME();

            nativeStopNodeDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NodeName);

            // make it Utility.To...
            nativeStopNodeDescriptionUsingNodeName.NodeInstance = (ulong)this.NodeInstance;

            switch (this.CompletionMode)
            {
                case CompletionMode.DoNotVerify:
                    nativeStopNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_DO_NOT_VERIFY;
                    break;
                case CompletionMode.Verify:
                    nativeStopNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_VERIFY;
                    break;
                default:
                    nativeStopNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_INVALID;
                    break;
            }

            var raw = pinCollection.AddBlittable(nativeStopNodeDescriptionUsingNodeName);

            var recreated = CreateFromNative(raw);

            return raw;
        }

        internal static unsafe StopNodeDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            var native = *(NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            BigInteger nodeInstance = new BigInteger(native.NodeInstance);

            CompletionMode completionMode;
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

            return new StopNodeDescriptionUsingNodeName(nodeName, nodeInstance, completionMode);
        }
    }
}
