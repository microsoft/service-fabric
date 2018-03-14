// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ServiceModel;
using namespace Common;
using namespace Transport;
using namespace Hosting2;
using namespace FabricHost;

RealConsole console;

const StringLiteral TraceType("FabricHost");

FabricActivatorService::FabricActivatorService(
    bool runAsService,
    bool activateHidden,
    bool skipFabricSetup) 
    : ServiceBase(FabricHost::Constants::ServiceName),
    runAsService_(runAsService),
    activateHidden_(activateHidden),
    skipFabricSetup_(skipFabricSetup),
    stopping_(false),
    activationManager_()
{
}

void FabricActivatorService::OnStart(Common::StringCollection const &)
{
    Trace.WriteInfo(TraceType, "FabricActivatorService starting ...");

    if (runAsService_)
    {
        ServiceModeStartWait();
    }
    else
    {
        ConsoleModeStartWait();
    }
}

#if !defined(PLATFORM_UNIX)
void FabricActivatorService::OnPreshutdown()
{
    Trace.WriteInfo(TraceType, "Received Preshutdown notification, stopping ...");
    if (Hosting2::FabricHostConfig::GetConfig().EnableRestartManagement && runAsService_)
    {
        SetServiceState(SERVICE_STOP_PENDING, 4, 30000);
        Threadpool::Post([this]()
        {
            this->DisableNode();
        });
    }
    else
    {
        OnStop(false);
    }
}
#endif

void FabricActivatorService::OnStop(bool shutdown)
{
    Trace.WriteInfo(TraceType, "service stopping (shutdown = {0}) ...", shutdown);
    if(stopping_)
    {
        Trace.WriteInfo(TraceType, "service is already in stopping process");
        return;
    }
    stopping_ = true;
    if (runAsService_)
    {
        ServiceModeStopWait();
    }
    else
    {
        ConsoleModeStopWait();
    }
}

void FabricActivatorService::OnPause()
{
    Assert::CodingError("{0} does not support pause", FabricHost::Constants::ServiceName);
}

void FabricActivatorService::OnContinue()
{
    Assert::CodingError("{0} does not support continue", FabricHost::Constants::ServiceName);
}

void FabricActivatorService::ServiceModeStartWait()
{
    StartActivationManager();
}

void FabricActivatorService::ConsoleModeStartWait()
{
    StartActivationManager();
}

void FabricActivatorService::ServiceModeStopWait()
{
    StopActivationManager();
}

void FabricActivatorService::ConsoleModeStopWait()
{
    console.WriteLine(StringResource::Get(IDS_FABRICHOST_CLOSING));
    StopActivationManager();
    console.WriteLine(StringResource::Get(IDS_FABRICHOST_CLOSED));
}

void FabricActivatorService::OnOpenComplete(AsyncOperationSPtr operation)
{
    ErrorCode error = activationManager_->EndOpen(operation);
    Trace.WriteTrace(error.ToLogLevel(),
        TraceType,
        "FabricActivatorManager  open returned {0}",
        error);
    if(!error.IsSuccess())
    {
        ::ExitProcess(1);
    }
}

void FabricActivatorService::CreateAndOpen(AutoResetEvent & openEvent)
{
    activationManager_ = make_shared<FabricActivationManager>(activateHidden_, skipFabricSetup_);
    activationManager_->BeginOpen(Hosting2::FabricHostConfig::GetConfig().StartTimeout,
        [&openEvent, this] (AsyncOperationSPtr const& operation) 
    {
        this->OnOpenComplete(operation); 
        openEvent.Set(); 
    },
        activationManager_->CreateAsyncOperationRoot()
        );
}

void FabricActivatorService::StartActivationManager()
{
    AutoResetEvent openEvent;

    Threadpool::Post([this, &openEvent]()
    {
        this->CreateAndOpen(openEvent);
    });

    openEvent.WaitOne();
}

void FabricActivatorService::OnCloseCompleted(AsyncOperationSPtr operation)
{
    ErrorCode error = activationManager_->EndClose(operation);
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "FabricActivatorManager close returned {0}",
        error);
}

void FabricActivatorService::Close(AutoResetEvent & closeEvent)
{
    Trace.WriteInfo(
        TraceType,
        "FabricActivatorService close with timeout {0} ",
        Hosting2::FabricHostConfig::GetConfig().StopTimeout);
    activationManager_->BeginClose(Hosting2::FabricHostConfig::GetConfig().StopTimeout,
        [&closeEvent, this](AsyncOperationSPtr operation)
    {
        this->OnCloseCompleted(operation);
        closeEvent.Set();
    },
        activationManager_->CreateAsyncOperationRoot());
}

void FabricActivatorService::StopActivationManager()
{
    Trace.WriteInfo(
        TraceType,
        "FabricActivatorService StopActivationManager called {0}",
        Hosting2::FabricHostConfig::GetConfig().StopTimeout);
    AutoResetEvent closeEvent;
    Threadpool::Post([this, &closeEvent]()
    {
        this->Close(closeEvent);
    });
    closeEvent.WaitOne();
}

#if !defined(PLATFORM_UNIX)
void FabricActivatorService::DisableNode()
{
    AutoResetEvent disableEvent;
    int checkpoint = 5;
    SetServiceState(SERVICE_STOP_PENDING, checkpoint++, 30000);
    Trace.WriteInfo(
        TraceType,
        "FabricActivatorService DisableNode called with timeout {0}",
        FabricHost::Constants::NodeDisableWaitTimeInMinutes);

    Threadpool::Post([this, &disableEvent]()
    {
        auto asyncPtr = activationManager_->FabricRestartManagerObj->BeginNodeDisable(
            TimeSpan::FromMinutes(FabricHost::Constants::NodeDisableWaitTimeInMinutes),
            [&disableEvent, this](AsyncOperationSPtr operation)
        {
            this->activationManager_->FabricRestartManagerObj->EndNodeDisable(operation);
            disableEvent.Set();
        },
            activationManager_->CreateAsyncOperationRoot());
        if (asyncPtr->CompletedSynchronously)
        {
            this->activationManager_->FabricRestartManagerObj->EndNodeDisable(asyncPtr);
            disableEvent.Set();
        }
    });

    SetServiceState(SERVICE_STOP_PENDING, checkpoint++, 30000);
    while (!disableEvent.WaitOne(TimeSpan::FromSeconds(15)))
    {
        Trace.WriteInfo(TraceType, "FabricActivatorService DisableNode, Still Waiting {0}", checkpoint);
        SetServiceState(SERVICE_STOP_PENDING, checkpoint++, 30000);
    }

    Trace.WriteInfo(TraceType, "Node disabled:{0}", checkpoint);
    SetServiceState(SERVICE_STOP_PENDING, checkpoint++, 30000);
    OnStop(false);

    Trace.WriteInfo(TraceType, "Stopping service...");
    SetServiceState(SERVICE_STOPPED, checkpoint++);
}
#endif
