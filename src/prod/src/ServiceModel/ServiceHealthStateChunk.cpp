// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceHealthStateChunk)

StringLiteral const TraceSource("ServiceHealthStateChunk");

ServiceHealthStateChunk::ServiceHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , serviceName_()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
    , partitionHealthStateChunks_()
{
}

ServiceHealthStateChunk::ServiceHealthStateChunk(
    std::wstring const & serviceName,
    FABRIC_HEALTH_STATE healthState,
    PartitionHealthStateChunkList && partitionHealthStateChunks)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , serviceName_(serviceName)
    , healthState_(healthState)
    , partitionHealthStateChunks_(move(partitionHealthStateChunks))
{
}

ServiceHealthStateChunk::~ServiceHealthStateChunk()
{
}

ErrorCode ServiceHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_SERVICE_HEALTH_STATE_CHUNK & publicServiceHealthStateChunk) const
{
    publicServiceHealthStateChunk.ServiceName = heap.AddString(serviceName_);

    publicServiceHealthStateChunk.HealthState = healthState_;

    // PartitionHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST>();
        auto error = partitionHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "PartitionHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicServiceHealthStateChunk.PartitionHealthStateChunks = publicInner.GetRawPointer();
    }

    return ErrorCode::Success();
}

ErrorCode ServiceHealthStateChunk::FromPublicApi(FABRIC_SERVICE_HEALTH_STATE_CHUNK const & publicServiceHealthStateChunk)
{
    auto hr = StringUtility::LpcwstrToWstring(publicServiceHealthStateChunk.ServiceName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    healthState_ = publicServiceHealthStateChunk.HealthState;

    auto error = partitionHealthStateChunks_.FromPublicApi(publicServiceHealthStateChunk.PartitionHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}

void ServiceHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ServiceHealthStateChunk({0}: {1}, {2} partitions)", serviceName_, healthState_, partitionHealthStateChunks_.Count);
}


