// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace std::placeholders;
using namespace TracePoints;

// {019DAA0F-E775-471A-AA85-49363C18E179}
GUID const TracePointsState::TracePointsProviderId = { 0x19daa0f, 0xe775, 0x471a, { 0xaa, 0x85, 0x49, 0x36, 0x3c, 0x18, 0xe1, 0x79 } };

TracePointsState::TracePointsState()
    : regHandle_(RegisterProvider()),
    eventSource_(nullptr),
    tracePointMap_(nullptr),
    regHandleMap_(nullptr),
    tracePointServer_(nullptr)
{
}

TracePointsState::~TracePointsState()
{
    EventUnregister(regHandle_);
}

void TracePointsState::Setup(EventWriteFunc eventWrite)
{
    eventSource_ = UniquePtr::Create<ServerEventSource>(regHandle_, eventWrite);
    tracePointMap_ = UniquePtr::Create<ProviderMap>(*eventSource_);
    regHandleMap_ = UniquePtr::Create<RegHandleMap>();
    tracePointServer_ = UniquePtr::Create<TracePointServer>(*tracePointMap_, *eventSource_);

    tracePointServer_->Start();
}

void TracePointsState::Cleanup()
{
    tracePointServer_->Stop();

    tracePointServer_ = nullptr;
    regHandleMap_ = nullptr;
    tracePointMap_ = nullptr;
    eventSource_ = nullptr;
}

void TracePointsState::OnEventWrite(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    ProviderId providerId = regHandleMap_->Get(RegHandle);
    TracePointData tracePointData;

    if (tracePointMap_->TryGetEventMapEntry(providerId, EventDescriptor->Id, tracePointData))
    {            
        eventSource_->InvokingUserFunction(&providerId, EventDescriptor->Id);
        tracePointData.action(EventDescriptor, UserDataCount, UserData, tracePointData.context);
    }
}

void TracePointsState::OnEventRegister(LPCGUID ProviderId, PREGHANDLE RegHandle)
{
    eventSource_->EventProviderRegistered(ProviderId, RegHandle);
    regHandleMap_->Add(*RegHandle, *ProviderId);
}

void TracePointsState::OnEventUnregister(REGHANDLE RegHandle)
{
    eventSource_->EventProviderUnregistered(RegHandle);
    regHandleMap_->Remove(RegHandle);
}

REGHANDLE TracePointsState::RegisterProvider()
{
    REGHANDLE regHandle;
    ULONG error = EventRegister(&TracePointsProviderId, nullptr, nullptr, &regHandle);
    if (error != ERROR_SUCCESS)
    {
        throw Error::LastError("Failed to register the TracePoints trace provider.");
    }

    return regHandle;
}
