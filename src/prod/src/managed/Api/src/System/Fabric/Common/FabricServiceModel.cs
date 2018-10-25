// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric;
    using System.Fabric.Interop;

    internal static class FabricServiceModel
    {
        public static NodeId GetNodeIdFromNodeName(
            string nodeName, 
            string rolesForWhichToUseV1Generator, 
            bool useV2NodeIdGenerator, 
            string nodeIdGeneratorVersion)
        {
            return Utility.WrapNativeSyncInvokeInMTA(
                () =>
                    FabricServiceModel.GetNodeIdFromNodeNameHelper(
                        nodeName,
                        rolesForWhichToUseV1Generator,
                        useV2NodeIdGenerator,
                        nodeIdGeneratorVersion),
                        "FabricServiceModel.GetNodeIdFromNodeName");
        }

        private static NodeId GetNodeIdFromNodeNameHelper(
            string nodeName,
            string rolesForWhichToUseV1Generator,
            bool useV2NodeIdGenerator,
            string nodeIdGeneratorVersion)
        {
            using (var pin = new PinCollection())
            {
                NativeTypes.FABRIC_NODE_ID nativeNodeId;

                NativeCommon.FabricGetNodeIdFromNodeName(
                    pin.AddBlittable(nodeName),
                    pin.AddBlittable(rolesForWhichToUseV1Generator),
                    NativeTypes.ToBOOLEAN(useV2NodeIdGenerator),
                    pin.AddBlittable(nodeIdGeneratorVersion),
                    out nativeNodeId);

                return NativeTypes.FromNativeNodeId(nativeNodeId);
            }
        }
    }
}