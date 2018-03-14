// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestStatefulServicePartition.h"

namespace ReplicationUnitTest
{
    using std::wstring;

    ComTestStatefulServicePartition::ComTestStatefulServicePartition(
        Common::Guid const & partitionId)
        :  
        IFabricStatefulServicePartition(),
        ComUnknownBase()
    {
        singletonPartitionInfo_.Id = partitionId.AsGUID();
        singletonPartitionInfo_.Reserved = NULL;
        partitionInfo_.Kind = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
        partitionInfo_.Value = &singletonPartitionInfo_;
        readStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
        writeStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
    }

    HRESULT __stdcall ComTestStatefulServicePartition::GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const ** bufferedValue)
    {
        *bufferedValue = &partitionInfo_;
        return S_OK;
    }

    HRESULT ComTestStatefulServicePartition::GetReadStatus(
        /*[out, retval]*/ FABRIC_SERVICE_PARTITION_ACCESS_STATUS * readStatus)
    {
        *readStatus = readStatus_;
        return S_OK;
    }

    HRESULT ComTestStatefulServicePartition::GetWriteStatus(
        /*[out, retval]*/ FABRIC_SERVICE_PARTITION_ACCESS_STATUS * writeStatus)
    {
        *writeStatus = writeStatus_;
        return S_OK;
    }

    HRESULT ComTestStatefulServicePartition::CreateReplicator(
        __in ::IFabricStateProvider *stateProvider,
        __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
        __out ::IFabricReplicator **replicator,
        __out ::IFabricStateReplicator **stateReplicator)
    {
        UNREFERENCED_PARAMETER(replicatorSettings);
        UNREFERENCED_PARAMETER(replicator);
        UNREFERENCED_PARAMETER(stateProvider);
        UNREFERENCED_PARAMETER(stateReplicator);

        return E_FAIL;
    }

    HRESULT ComTestStatefulServicePartition::ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics)
    {
        UNREFERENCED_PARAMETER(metricCount);
        UNREFERENCED_PARAMETER(metrics);

        return E_FAIL;
    }

    HRESULT ComTestStatefulServicePartition::ReportFault(FABRIC_FAULT_TYPE faultType)
    {
        UNREFERENCED_PARAMETER(faultType);

        return E_NOTIMPL;
    }

    void ComTestStatefulServicePartition::SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus)
    {
        readStatus_ = readStatus;
    }

    void ComTestStatefulServicePartition::SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
    {
        writeStatus_ = writeStatus;
    }

}
