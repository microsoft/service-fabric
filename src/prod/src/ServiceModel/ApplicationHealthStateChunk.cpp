// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ApplicationHealthStateChunk)

StringLiteral const TraceSource("ApplicationHealthStateChunk");

ApplicationHealthStateChunk::ApplicationHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , applicationName_()
    , applicationTypeName_()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
    , serviceHealthStateChunks_()
    , deployedApplicationHealthStateChunks_()
{
}

ApplicationHealthStateChunk::ApplicationHealthStateChunk(
    std::wstring const & applicationName,
    std::wstring const & applicationTypeName,
    FABRIC_HEALTH_STATE healthState,
    ServiceHealthStateChunkList && serviceHealthStateChunks,
    DeployedApplicationHealthStateChunkList && deployedApplicationHealthStateChunks)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , applicationName_(applicationName)
    , applicationTypeName_(move(applicationTypeName))
    , healthState_(healthState)
    , serviceHealthStateChunks_(move(serviceHealthStateChunks))
    , deployedApplicationHealthStateChunks_(move(deployedApplicationHealthStateChunks))
{
}

ApplicationHealthStateChunk::~ApplicationHealthStateChunk()
{
}

ErrorCode ApplicationHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_APPLICATION_HEALTH_STATE_CHUNK & publicApplicationHealthStateChunk) const
{
    publicApplicationHealthStateChunk.ApplicationName = heap.AddString(applicationName_);

    publicApplicationHealthStateChunk.HealthState = healthState_;

    // ServiceHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_SERVICE_HEALTH_STATE_CHUNK_LIST>();
        auto error = serviceHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ServiceHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicApplicationHealthStateChunk.ServiceHealthStateChunks = publicInner.GetRawPointer();
    }

    // DeployedApplicationHealthStateChunks
    {
        auto publicInner = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST>();
        auto error = deployedApplicationHealthStateChunks_.ToPublicApi(heap, *publicInner);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedApplicationHealthStateChunks::ToPublicApi error: {0}", error);
            return error;
        }

        publicApplicationHealthStateChunk.DeployedApplicationHealthStateChunks = publicInner.GetRawPointer();
    }

    auto ex1 = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATE_CHUNK_EX1>();
    ex1->ApplicationTypeName = heap.AddString(applicationTypeName_);
    publicApplicationHealthStateChunk.Reserved = ex1.GetRawPointer();
    
    return ErrorCode::Success();
}

ErrorCode ApplicationHealthStateChunk::FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_CHUNK const & publicApplicationHealthStateChunk)
{
    auto hr = StringUtility::LpcwstrToWstring(publicApplicationHealthStateChunk.ApplicationName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    healthState_ = publicApplicationHealthStateChunk.HealthState;

    auto error = serviceHealthStateChunks_.FromPublicApi(publicApplicationHealthStateChunk.ServiceHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = deployedApplicationHealthStateChunks_.FromPublicApi(publicApplicationHealthStateChunk.DeployedApplicationHealthStateChunks);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (publicApplicationHealthStateChunk.Reserved != NULL)
    {
        auto ex1 = static_cast<FABRIC_APPLICATION_HEALTH_STATE_CHUNK_EX1*>(publicApplicationHealthStateChunk.Reserved);
        hr = StringUtility::LpcwstrToWstring(ex1->ApplicationTypeName, true, 0, ParameterValidator::MaxStringSize, applicationTypeName_);
        if (FAILED(hr))
        {
            Trace.WriteInfo(TraceSource, "Error parsing application type name in FromPublicAPI: {0}", hr);
            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCode::Success();
}

void ApplicationHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ApplicationHealthStateChunk({0}: app type {1}, {2}, {3} services, {4} deployed apps)", applicationName_, applicationTypeName_, healthState_, serviceHealthStateChunks_.Count, deployedApplicationHealthStateChunks_.Count);
}
