// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedServicePackageHealthStateChunk)

StringLiteral const TraceSource("DeployedServicePackageHealthStateChunk");

DeployedServicePackageHealthStateChunk::DeployedServicePackageHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

DeployedServicePackageHealthStateChunk::DeployedServicePackageHealthStateChunk(
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    FABRIC_HEALTH_STATE healthState)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , Common::ISizeEstimator()
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , healthState_(healthState)
{
}

DeployedServicePackageHealthStateChunk::~DeployedServicePackageHealthStateChunk()
{
}

ErrorCode DeployedServicePackageHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK & publicDeployedServicePackageHealthStateChunk) const
{
    publicDeployedServicePackageHealthStateChunk.ServiceManifestName = heap.AddString(serviceManifestName_);

    publicDeployedServicePackageHealthStateChunk.HealthState = healthState_;

    auto healthStateChunkEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_EX1>();

    healthStateChunkEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    healthStateChunkEx1->Reserved = nullptr;
    publicDeployedServicePackageHealthStateChunk.Reserved = healthStateChunkEx1.GetRawPointer();

    return ErrorCode::Success();
}

ErrorCode DeployedServicePackageHealthStateChunk::FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK const & publicDeployedServicePackageHealthStateChunk)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedServicePackageHealthStateChunk.ServiceManifestName, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceManifestName_);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing service manifest name in FromPublicAPI: {0}", error);
        return error;
    }

    healthState_ = publicDeployedServicePackageHealthStateChunk.HealthState;

    if (publicDeployedServicePackageHealthStateChunk.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto healthStateChunkEx1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_EX1*)publicDeployedServicePackageHealthStateChunk.Reserved;

	error = StringUtility::LpcwstrToWstring2(healthStateChunkEx1->ServicePackageActivationId, true, servicePackageActivationId_);
	if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error parsing ServicePackageActivationId in FromPublicAPI: {0}", error);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void DeployedServicePackageHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("DeployedServicePackageHealthStateChunk({0}[{1}]: {2})", serviceManifestName_, servicePackageActivationId_, healthState_);
}
