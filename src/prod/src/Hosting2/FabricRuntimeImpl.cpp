// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

FabricRuntimeImpl::FabricRuntimeImpl(
    ComponentRoot const & parent,
    ApplicationHost & host,
    FabricRuntimeContext && context,
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler)
    : ComponentRoot(),
    runtimeContextLock_(),
    context_(move(context)),
    runtimeId_(context_.RuntimeId),
    parent_(parent.CreateComponentRoot()),
    host_(host),
    serviceFactoryManager_(),
    fabricExitHandler_(fabricExitHandler)
{
    serviceFactoryManager_ = make_unique<ServiceFactoryManager>(
        *this, /* ComponentRoot */
        *this);/* FabricRuntimeImpl */
   
    this->SetTraceId(L"RT-" + runtimeId_ + L"@" + host.TraceId);
}

FabricRuntimeImpl::~FabricRuntimeImpl()
{
    WriteNoise(
        "FabricRuntimeImpl",
        TraceId,
        "FabricRuntimeImpl::Destructed Id={0}",
        RuntimeId);
}

void FabricRuntimeImpl::OnFabricProcessExited()
{
    if (fabricExitHandler_.GetRawPointer() != NULL)
    {
        auto fabricExitHandler = move(fabricExitHandler_);
        wstring const & traceId = TraceId;
        wstring const & runtimeId = RuntimeId;
        Threadpool::Post(
            [fabricExitHandler, traceId, runtimeId] () mutable
            {            
                ASSERT_IF(
                    fabricExitHandler.GetRawPointer() == NULL,
                    "Exit handler is empty. Double release made");
                TraceNoise(
                    TraceTaskCodes::Hosting,
                    "FabricRuntimeImpl",
                    traceId,
                    "Invoking FabricProcessExitHandler. RuntimeId={0}",
                    runtimeId);

                fabricExitHandler->FabricProcessExited();
                fabricExitHandler.Release();            
            });
    }
}

void FabricRuntimeImpl::UpdateFabricRuntimeContext(FabricRuntimeContext const& fabricRuntimeContext)
{
    AcquireWriteLock grab(runtimeContextLock_);
    context_ = fabricRuntimeContext;  
}

FabricRuntimeContext FabricRuntimeImpl::GetRuntimeContext()
{
    AcquireReadLock grab(runtimeContextLock_);
    return context_;  
};
