// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility.InProc
{
    using System;
    using System.Runtime.InteropServices;

    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public class NodeConfig
    {
        public readonly WinFabricNodeId NodeId;

        [MarshalAs(UnmanagedType.BStr)]
        public readonly string Address;

        [MarshalAs(UnmanagedType.BStr)]
        public readonly string LeaseAgentAddress;

        [MarshalAs(UnmanagedType.BStr)]
        public readonly string WorkingDir;

        public NodeConfig(WinFabricNodeId nodeId, string address, string leaseAgentAddress, string workingDir)
        {
            if (address == null)
            {
                throw new ArgumentNullException("address");
            }

            if (leaseAgentAddress == null)
            {
                throw new ArgumentNullException("leaseAgentAddress");
            }

            this.NodeId = nodeId;
            this.Address = address;
            this.LeaseAgentAddress = leaseAgentAddress;
            this.WorkingDir = workingDir;
        }
    }
}