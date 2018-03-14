// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;

ChaosEventBase::ChaosEventBase()
: kind_(FABRIC_CHAOS_EVENT_KIND_INVALID)
, timeStampUtc_()
{
}

ChaosEventBase::ChaosEventBase(FABRIC_CHAOS_EVENT_KIND kind)
: kind_(kind)
, timeStampUtc_()
{
}

ChaosEventBase::ChaosEventBase(
    FABRIC_CHAOS_EVENT_KIND kind,
    DateTime timeStampUtc)
: kind_(kind)
, timeStampUtc_(timeStampUtc)
{
}

ChaosEventBase::ChaosEventBase(ChaosEventBase && other)
: kind_(move(other.kind_))
, timeStampUtc_(move(other.timeStampUtc_))
{
}

ChaosEventBase & ChaosEventBase::operator = (ChaosEventBase && other)
{
    if (this != &other)
    {
        kind_ = move(other.kind_);
        timeStampUtc_ = move(other.timeStampUtc_);
    }

    return *this;
}

ChaosEventBase::~ChaosEventBase()
{
}

ErrorCode ChaosEventBase::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_EVENT & publicChaosEvent) const
{
    UNREFERENCED_PARAMETER(heap);
    publicChaosEvent.Kind = FABRIC_CHAOS_EVENT_KIND_INVALID;
    publicChaosEvent.Value = NULL;
    return ErrorCode::Success();
}

ErrorCode ChaosEventBase::FromPublicApi(
    FABRIC_CHAOS_EVENT const & publicChaosEvent)
{
    UNREFERENCED_PARAMETER(publicChaosEvent);
    kind_ = FABRIC_CHAOS_EVENT_KIND_INVALID;
    return ErrorCode(ErrorCodeValue::InvalidArgument);
}

Serialization::IFabricSerializable * ChaosEventBase::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(FABRIC_CHAOS_EVENT_KIND))
    {
        return nullptr;
    }

    FABRIC_CHAOS_EVENT_KIND kind = *(reinterpret_cast<FABRIC_CHAOS_EVENT_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

ChaosEventBase * ChaosEventBase::CreateNew(FABRIC_CHAOS_EVENT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_CHAOS_EVENT_KIND_STARTED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_STARTED>::CreateNew();
    case FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS>::CreateNew();
    case FABRIC_CHAOS_EVENT_KIND_WAITING:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_WAITING>::CreateNew();
    case FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED>::CreateNew();
    case FABRIC_CHAOS_EVENT_KIND_TEST_ERROR:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_TEST_ERROR>::CreateNew();
    case FABRIC_CHAOS_EVENT_KIND_STOPPED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_STOPPED>::CreateNew();

    default:
        return new ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_INVALID);
    }
}

ChaosEventBaseSPtr ChaosEventBase::CreateSPtr(FABRIC_CHAOS_EVENT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_CHAOS_EVENT_KIND_STARTED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_STARTED>::CreateSPtr();
    case FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS>::CreateSPtr();
    case FABRIC_CHAOS_EVENT_KIND_WAITING:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_WAITING>::CreateSPtr();
    case FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED>::CreateSPtr();
    case FABRIC_CHAOS_EVENT_KIND_TEST_ERROR:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_TEST_ERROR>::CreateSPtr();
    case FABRIC_CHAOS_EVENT_KIND_STOPPED:
        return ChaosEventSerializationTypeActivator<FABRIC_CHAOS_EVENT_KIND_STOPPED>::CreateSPtr();

    default:
        return make_shared<ChaosEventBase>(FABRIC_CHAOS_EVENT_KIND_INVALID);
    }
}

NTSTATUS ChaosEventBase::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}
