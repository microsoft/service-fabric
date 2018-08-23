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

    internal sealed class StartNodeDescriptionUsingNodeName
    {
        public StartNodeDescriptionUsingNodeName(
            string nodeName,
            BigInteger nodeInstance,
            string ipAddressOrFQDN,
            int clusterConnectionPort,
            CompletionMode completionMode)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.IpAddressOrFQDN = ipAddressOrFQDN;
            this.ClusterConnectionPort = clusterConnectionPort;
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

        public string IpAddressOrFQDN
        {
            get;
            internal set;
        }

        public int ClusterConnectionPort
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
            return string.Format(CultureInfo.InvariantCulture, "NodeName: {0}, NodeInstance: {1}, IpAddressOrFQDN = {2}, CluserConnectionPort = {3}, Completion mode: {4}",
                this.NodeName,
                this.NodeInstance.ToString(),
                this.IpAddressOrFQDN,
                this.ClusterConnectionPort,
                this.CompletionMode);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStartNodeDescriptionUsingNodeName = new NativeTypes.FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME();

            nativeStartNodeDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NodeName);

            // make it Utility.To...
            nativeStartNodeDescriptionUsingNodeName.NodeInstance = (ulong)this.NodeInstance;

            nativeStartNodeDescriptionUsingNodeName.IPAddressOrFQDN = pinCollection.AddObject(this.IpAddressOrFQDN);

            nativeStartNodeDescriptionUsingNodeName.ClusterConnectionPort = (uint)this.ClusterConnectionPort;

            switch (this.CompletionMode)
            {
                case CompletionMode.DoNotVerify:
                    nativeStartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_DO_NOT_VERIFY;
                    break;
                case CompletionMode.Verify:
                    nativeStartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_VERIFY;
                    break;
                default:
                    nativeStartNodeDescriptionUsingNodeName.CompletionMode = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_INVALID;
                    break;
            }

            var raw = pinCollection.AddBlittable(nativeStartNodeDescriptionUsingNodeName);

            var recreated = CreateFromNative(raw);

            return raw;
        }

        internal static unsafe StartNodeDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            var native = *(NativeTypes.FABRIC_START_NODE_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            BigInteger nodeInstance = new BigInteger(native.NodeInstance);

            string ipAddressOrFQDN = NativeTypes.FromNativeString(native.IPAddressOrFQDN);

            int clusterConnectionPort = (int)native.ClusterConnectionPort;

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

            return new StartNodeDescriptionUsingNodeName(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, completionMode);
        }
    }
}
