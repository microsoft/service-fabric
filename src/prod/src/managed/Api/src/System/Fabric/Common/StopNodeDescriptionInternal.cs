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

    internal sealed class StopNodeDescriptionInternal
    {
        public StopNodeDescriptionInternal(
            string nodeName,
            BigInteger nodeInstance,
            int stopDurationInSeconds)            
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.StopDurationInSeconds = stopDurationInSeconds;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public BigInteger NodeInstance
        {
            get;
            private set;
        }

        public int StopDurationInSeconds
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NodeName: {0}, NodeInstance: {1}, StopDurationInSeconds: {2}",
                this.NodeName,
                this.NodeInstance.ToString(),
                this.StopDurationInSeconds);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStopNodeDescriptionUsingNodeName = new NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_INTERNAL();

            nativeStopNodeDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NodeName);

            nativeStopNodeDescriptionUsingNodeName.NodeInstance = (ulong)this.NodeInstance;
            nativeStopNodeDescriptionUsingNodeName.StopDurationInSeconds = this.StopDurationInSeconds;
         
            var raw = pinCollection.AddBlittable(nativeStopNodeDescriptionUsingNodeName);

            return raw;
        }

        internal static unsafe StopNodeDescriptionInternal CreateFromNative(IntPtr nativeRaw)
        {
            var native = *(NativeTypes.FABRIC_STOP_NODE_DESCRIPTION_INTERNAL*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            BigInteger nodeInstance = new BigInteger(native.NodeInstance);

            int stopDurationInSeconds = native.StopDurationInSeconds;

            return new StopNodeDescriptionInternal(nodeName, nodeInstance, stopDurationInSeconds);
        }
    }
}
