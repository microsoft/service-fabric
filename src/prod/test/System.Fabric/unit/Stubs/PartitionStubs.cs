// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    
    using System.Fabric.Interop;

    class StatefulPartitionStubBase : NativeRuntime.IFabricStatefulServicePartition3
    {
        public virtual NativeRuntime.IFabricStateReplicator CreateReplicator(NativeRuntime.IFabricStateProvider stateProvider, IntPtr fabricReplicatorSettings, out NativeRuntime.IFabricReplicator replicator)
        {
            throw new NotImplementedException();
        }

        public virtual NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus()
        {
            throw new NotImplementedException();
        }

        public virtual NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus()
        {
            throw new NotImplementedException();
        }

        public virtual IntPtr GetPartitionInfo()
        {
            throw new NotImplementedException();
        }

        public virtual void ReportLoad(uint metricCount, IntPtr metrics)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportFault(NativeTypes.FABRIC_FAULT_TYPE faultType)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportMoveCost(NativeTypes.FABRIC_MOVE_COST moveCost)
        {
            throw new NotImplementedException();
        }
        public virtual void ReportReplicaHealth(IntPtr healthInfo)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportPartitionHealth(IntPtr healthInfo)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportReplicaHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportPartitionHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            throw new NotImplementedException();
        }
    }

    class StatelessPartitionStubBase : NativeRuntime.IFabricStatelessServicePartition3
    {

        public virtual IntPtr GetPartitionInfo()
        {
            throw new NotImplementedException();
        }

        public virtual void ReportLoad(uint metricCount, IntPtr metrics)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportFault(NativeTypes.FABRIC_FAULT_TYPE faultType)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportMoveCost(NativeTypes.FABRIC_MOVE_COST moveCost)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportInstanceHealth(IntPtr healthInfo)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportPartitionHealth(IntPtr healthInfo)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportInstanceHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            throw new NotImplementedException();
        }

        public virtual void ReportPartitionHealth2(IntPtr healthInfo, IntPtr sendOptions)
        {
            throw new NotImplementedException();
        }
    }
}