// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Numerics;

    internal sealed class StartNodeTransitionDescription
    {
        public StartNodeTransitionDescription(
            Guid operationId,
            NodeTransitionType nodeTransitionType,
            string nodeName,
            BigInteger nodeInstanceId)            
        {            
            this.OperationId = operationId;
            this.NodeTransitionType = nodeTransitionType;
            this.NodeName = nodeName;
            this.NodeInstanceId = nodeInstanceId;
        }

        public Guid OperationId
        {
            get;
            internal set;
        }

        public NodeTransitionType NodeTransitionType
        {
            get;
            internal set;
        }

        public string NodeName
        {
            get;
            internal set;
        }

        public BigInteger NodeInstanceId
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "OperationId: {0}, NodeName: {1}, NodeInstanceId: {2}",
                this.OperationId,
                this.NodeName,
                this.NodeInstanceId);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStartNodeTransitionDescription = new NativeTypes.FABRIC_START_NODE_TRANSITION_DESCRIPTION();

            nativeStartNodeTransitionDescription.OperationId = this.OperationId;
            nativeStartNodeTransitionDescription.NodeTransitionType = (NativeTypes.FABRIC_NODE_TRANSITION_TYPE)this.NodeTransitionType;
            nativeStartNodeTransitionDescription.NodeName = pinCollection.AddBlittable(this.NodeName);
            nativeStartNodeTransitionDescription.NodeInstanceId = (ulong)this.NodeInstanceId;
            
            return pinCollection.AddBlittable(nativeStartNodeTransitionDescription);
        }

        internal static unsafe StartNodeTransitionDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_NODE_TRANSITION_DESCRIPTION native = *(NativeTypes.FABRIC_START_NODE_TRANSITION_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;
            string nodeName = NativeTypes.FromNativeString(native.NodeName);
            NodeTransitionType nodeTransitionType = (NodeTransitionType)native.NodeTransitionType;
            BigInteger nodeInstanceId = new BigInteger(native.NodeInstanceId);

            return new StartNodeTransitionDescription(operationId, nodeTransitionType, nodeName, nodeInstanceId);
        }
    }
}
