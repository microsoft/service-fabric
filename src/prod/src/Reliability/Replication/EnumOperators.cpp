// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::TextWriter;

TextWriter& operator << (__in TextWriter & w, FABRIC_OPERATION_TYPE const & val)
{
    switch (val)
    {
        case FABRIC_OPERATION_TYPE_NORMAL: 
            return w << "FABRIC_OPERATION_TYPE_NORMAL";
        default: 
            return w << "UNDEFINED FABRIC_OPERATION_TYPE=" << static_cast<int>(val);
    }
}

TextWriter& operator << (__in TextWriter & w, FABRIC_OPERATION_METADATA const & val)
{
    return w << val.Type << L"." << val.SequenceNumber;
}

TextWriter& operator << (__in TextWriter & w, FABRIC_SERVICE_PARTITION_ACCESS_STATUS const & val)
{
    switch (val)
    {
        case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING: 
            return w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING";
        case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY: 
            return w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY";
        case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM: 
            return w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM";
        case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED: 
            return w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED";
        default: 
            return w << "UNDEFINED FABRIC_SERVICE_PARTITION_ACCESS_STATUS=" << static_cast<int>(val);
    }
}

Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_REPLICA_SET_QUORUM_MODE const & val)
{
    switch(val)
    {
        case FABRIC_REPLICA_SET_QUORUM_ALL:
            return w << "FABRIC_REPLICA_SET_QUORUM_ALL";
        case FABRIC_REPLICA_SET_WRITE_QUORUM:
            return w << "FABRIC_REPLICA_SET_WRITE_QUORUM";
        default:
            return w << "UNDEFINED FABRIC_REPLICA_SET_QUORUM_MODE=" << static_cast<int>(val);
    }
}

Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_EPOCH const & val)
{
    w.Write("{0}.{1:X}", val.DataLossNumber, val.ConfigurationNumber);
    return w;
}

bool operator == (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b)
{ 
    return a.ConfigurationNumber == b.ConfigurationNumber &&
        a.DataLossNumber == b.DataLossNumber; 
}

bool operator != (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b) 
{ 
    return !(a == b); 
}

bool operator < (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b)
{
    return (a.DataLossNumber < b.DataLossNumber) ||
        (a.DataLossNumber == b.DataLossNumber && a.ConfigurationNumber < b.ConfigurationNumber);
}

bool operator <= (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b)
{
    return (a.DataLossNumber < b.DataLossNumber) ||
        (a.DataLossNumber == b.DataLossNumber && a.ConfigurationNumber <= b.ConfigurationNumber);
}

HRESULT ValidateOperation(
    Common::ComPointer<IFabricOperationData> const & operationPointer, 
    LONG maxSize,
    __out_opt std::wstring & errorMessage)
{
    LONG currentOperationSize = 0;
    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

    HRESULT hr = operationPointer->GetData(&bufferCount, &replicaBuffers);
    if (FAILED(hr)) { return hr; }
    if (bufferCount == 0)
    {
        if (replicaBuffers != NULL)
        {
            errorMessage = Common::wformatString("Operation buffer count is 0, but the buffer is not empty");
            return E_INVALIDARG;
        }
    }
    else
    {
        if (replicaBuffers == NULL)
        {
            errorMessage = Common::wformatString("Operation buffer count is {0}, but there are no buffers", bufferCount);
            return E_INVALIDARG;
        }
        for (ULONG i=0; i<bufferCount; i++)
        {
            if (replicaBuffers[i].Buffer == NULL)
            {
                errorMessage = Common::wformatString("Operation buffer {0} is NULL", i);
                return E_INVALIDARG;
            }
            currentOperationSize += replicaBuffers[i].BufferSize;
        }
    }

    if (currentOperationSize > maxSize)
    {
        errorMessage = Common::wformatString("Operation buffer size = {0} bytes exceeds MaxReplicationMessageSize = {1} bytes", currentOperationSize, maxSize);
        return FABRIC_E_REPLICATION_OPERATION_TOO_LARGE;
    }

    return S_OK;
}

HRESULT ValidateBatchOperation(
    Common::ComPointer<IFabricBatchOperationData> const & batchOperationPointer, 
    LONG maxSize,
    __out_opt std::wstring & errorMessage)
{
    LONG currentOperationSize = 0;
    FABRIC_SEQUENCE_NUMBER firstSequenceNumber = batchOperationPointer->get_FirstSequenceNumber();
    FABRIC_SEQUENCE_NUMBER lastSequenceNumber = batchOperationPointer->get_LastSequenceNumber();

    if (firstSequenceNumber <= Reliability::ReplicationComponent::Constants::InvalidLSN || 
        lastSequenceNumber <= Reliability::ReplicationComponent::Constants::InvalidLSN ||
        lastSequenceNumber < firstSequenceNumber)
    {
        errorMessage = Common::wformatString("Invalid FirstSequenceNumber = {0} and/or LastSequenceNumber = {1}", firstSequenceNumber, lastSequenceNumber);
        return E_INVALIDARG;
    }

    for (FABRIC_SEQUENCE_NUMBER lsn = firstSequenceNumber; lsn <= lastSequenceNumber; lsn ++)
    {
        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
    
        HRESULT hr = batchOperationPointer->GetData(lsn, &bufferCount, &replicaBuffers);
        if (FAILED(hr)) { return hr; }
        if (bufferCount == 0)
        {
            if (replicaBuffers != NULL)
            {
                errorMessage = Common::wformatString("BatchOperation buffer count is 0, but the buffer is not empty");
                return E_INVALIDARG;
            }
        }
        else
        {
            if (replicaBuffers == NULL)
            {
                errorMessage = Common::wformatString("BatchOperation buffer count is {0}, but there are no buffers", bufferCount);
                return E_INVALIDARG;
            }
            for (ULONG i=0; i<bufferCount; i++)
            {
                if (replicaBuffers[i].Buffer == NULL)
                {
                    errorMessage = Common::wformatString("BatchOperation buffer {0} is NULL", i);
                    return E_INVALIDARG;
                }
                currentOperationSize += replicaBuffers[i].BufferSize;
            }
        }
    }

    if (currentOperationSize > maxSize)
    {
        errorMessage = Common::wformatString("BatchOperation buffer size = {0} bytes exceeds MaxReplicationMessageSize = {1} bytes", currentOperationSize, maxSize);
        return FABRIC_E_REPLICATION_OPERATION_TOO_LARGE;
    }

    return S_OK;
}
