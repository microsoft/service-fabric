// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationHealthStateChunk)

StringLiteral const TraceSource("DeployedApplicationHealthStateChunk");

DeployedApplicationHealthStateChunk::DeployedApplicationHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , nodeName_()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
    , deployedServicePackageHealthStateChunks_()
{
}

DeployedApplicationHealthStateChunk::DeployedApplicationHealthStateChunk(
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE healthState,
    DeployedServicePackageHealthStateChunkList && deployedServicePackageHealthStateChunks)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , nodeName_(nodeName)
    , healthState_(healthState)
    , deployedServicePackageHealthStateChunks_(move(deployedServicePackageHealthStateChunks))
{
}

DeployedApplicationHealthStateChunk::~DeployedApplicationHealthStateChunk()
{
}

ErrorCode DeployedApplicationHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK & publicDeployedApplicationHealthStateChunk) const
{
    publicDeployedApplicationHealthStateChunk.NodeName = heap.AddString(nodeName_);

    publicDeployedApplicationHealthStateChunk.HealthState = healthState_;

    // DeployedServicePackageHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_LIST>();
        auto error = deployedServicePackageHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedServicePackageHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicDeployedApplicationHealthStateChunk.DeployedServicePackageHealthStateChunks = publicInner.GetRawPointer();
    }

    return ErrorCode::Success();
}

ErrorCode DeployedApplicationHealthStateChunk::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK const & publicDeployedApplicationHealthStateChunk)
{
    auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthStateChunk.NodeName, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    healthState_ = publicDeployedApplicationHealthStateChunk.HealthState;

    auto error = deployedServicePackageHealthStateChunks_.FromPublicApi(publicDeployedApplicationHealthStateChunk.DeployedServicePackageHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCode::Success();
}

void DeployedApplicationHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("DeployedApplicationHealthStateChunk({0}: {1}, {2} deployed service packages)", nodeName_, healthState_, deployedServicePackageHealthStateChunks_.Count);
}

