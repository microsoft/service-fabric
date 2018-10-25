// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(HealthEvaluationBase)

StringLiteral const TraceSource("HealthEvaluationBase");

HealthEvaluationBase::HealthEvaluationBase()
    : kind_(FABRIC_HEALTH_EVALUATION_KIND_INVALID)
    , description_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

HealthEvaluationBase::HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND kind)
    : kind_(kind)
    , description_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

HealthEvaluationBase::HealthEvaluationBase(
    FABRIC_HEALTH_EVALUATION_KIND kind,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : kind_(kind)
    , description_()
    , aggregatedHealthState_(aggregatedHealthState)
{
}

HealthEvaluationBase::~HealthEvaluationBase()
{
}

void HealthEvaluationBase::SetDescription()
{
    Assert::CodingError("SetDescription: Should be handled by derived classes");
}

void HealthEvaluationBase::GenerateDescription()
{
    this->SetDescription();
}

wstring HealthEvaluationBase::GetUnhealthyEvaluationDescription(wstring const & indent) const
{
    if (indent.empty())
    {
        return description_;
    }

    wstring result;
    StringWriter writer(result);
    // Add a newline to separate from the parent.
    writer.WriteLine();
    writer.Write("{0}{1}", indent, description_);
    return result;
}

Common::ErrorCode HealthEvaluationBase::GenerateDescriptionAndTrimIfNeeded(size_t maxAllowedSize, __inout size_t & currentSize)
{
    this->SetDescription();
    return this->TryAdd(maxAllowedSize, currentSize);
}

// Generate the description text and update currentSize with the string size.
// If the total size doesn't respect maxAllowedSize, return error.
Common::ErrorCode HealthEvaluationBase::TryAdd(
    size_t maxAllowedSize,
    __inout size_t & currentSize)
{
    currentSize += EstimateSize();
    if (currentSize >= maxAllowedSize)
    {
        // No need to generate a message, the caller uses the information to generate one
        Trace.WriteInfo(
            TraceSource,
            "Error adding description for evaluation. Description='{0}', maxSize={1}, currentSize={2}",
            description_,
            maxAllowedSize,
            currentSize);
        return ErrorCode(ErrorCodeValue::EntryTooLarge);
    }

    return ErrorCode::Success();
}

ErrorCode HealthEvaluationBase::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const
{
    UNREFERENCED_PARAMETER(heap);
    publicHealthEvaluation.Kind = FABRIC_HEALTH_EVALUATION_KIND_INVALID;
    publicHealthEvaluation.Value = NULL;
    return ErrorCode::Success();
}

ErrorCode HealthEvaluationBase::FromPublicApi(
    FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation)
{
    UNREFERENCED_PARAMETER(publicHealthEvaluation);
    kind_ = FABRIC_HEALTH_EVALUATION_KIND_INVALID;
    return ErrorCode(ErrorCodeValue::InvalidArgument);
}

int HealthEvaluationBase::GetUnhealthyPercent(size_t unhealthyCount, ULONG totalCount)
{
    if (totalCount == 0)
    {
        return 0;
    }

    return static_cast<int>(static_cast<float>(unhealthyCount) * 100.0f / static_cast<float>(totalCount));
}

Serialization::IFabricSerializable * HealthEvaluationBase::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(FABRIC_HEALTH_EVALUATION_KIND))
    {
        return nullptr;
    }

    FABRIC_HEALTH_EVALUATION_KIND kind = *(reinterpret_cast<FABRIC_HEALTH_EVALUATION_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

HealthEvaluationBase * HealthEvaluationBase::CreateNew(FABRIC_HEALTH_EVALUATION_KIND kind)
{
    switch (kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_EVENT>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_REPLICAS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SERVICES>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_NODES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_NODES>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_REPLICA:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_REPLICA>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_PARTITION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_PARTITION>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SERVICE>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_NODE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_NODE>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATION>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK>::CreateNew();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK>::CreateNew();

    default:
        return new HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND_INVALID);
    }
}

HealthEvaluationBaseSPtr HealthEvaluationBase::CreateSPtr(FABRIC_HEALTH_EVALUATION_KIND kind)
{
    switch (kind)
    {
    case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_EVENT>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_REPLICAS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SERVICES>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_NODES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_NODES>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_REPLICA:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_REPLICA>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_PARTITION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_PARTITION>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_SERVICE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_SERVICE>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_NODE:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_NODE>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_APPLICATION>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK>::CreateSPtr();
    case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK:
        return HealthEvaluationSerializationTypeActivator<FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK>::CreateSPtr();
    default:
        return make_shared<HealthEvaluationBase>(FABRIC_HEALTH_EVALUATION_KIND_INVALID);
    }
}

NTSTATUS HealthEvaluationBase::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

void HealthEvaluationBase::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.WriteLine();
    writer.Write("{0}: {1}: [{2}]", kind_, aggregatedHealthState_, description_);
}

void HealthEvaluationBase::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->HealthEvaluationBaseTrace(
        contextSequenceId,
        wformatString(kind_),
        wformatString(aggregatedHealthState_),
        description_);
}

wstring HealthEvaluationBase::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}
