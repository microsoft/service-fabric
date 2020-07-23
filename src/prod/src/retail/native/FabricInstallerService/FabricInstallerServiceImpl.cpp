// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ServiceModel;
using namespace Common;
using namespace FabricInstallerService;

RealConsole console;

const StringLiteral TraceType("FabricInstallerServiceImpl");

FabricInstallerServiceImpl::FabricInstallerServiceImpl(
    bool runAsService,
    bool autoclean)
    : ServiceBase(FabricInstallerService::Constants::ServiceName),
    runAsService_(runAsService),
    stopping_(false),
    autoclean_(autoclean),
    fabricUpgradeManager_()
{
}

void FabricInstallerServiceImpl::OnStart(Common::StringCollection const &)
{
    Trace.WriteInfo(TraceType, "FabricInstallerService starting ...");

    if (runAsService_)
    {
        ServiceModeStartWait();
    }
    else
    {
        ConsoleModeStartWait();
    }
}

void FabricInstallerServiceImpl::OnPreshutdown()
{
    Trace.WriteInfo(TraceType, "received Preshutdown notification, stopping ...");
    OnStop(false);
}

void FabricInstallerServiceImpl::OnStop(bool shutdown)
{
    Trace.WriteInfo(TraceType, "service stopping (shutdown = {0}) ...", shutdown);
    if (stopping_)
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

void FabricInstallerServiceImpl::OnPause()
{
    Assert::CodingError("{0} does not support pause", FabricInstallerService::Constants::ServiceName);
}

void FabricInstallerServiceImpl::OnContinue()
{
    Assert::CodingError("{0} does not support continue", FabricInstallerService::Constants::ServiceName);
}

void FabricInstallerServiceImpl::ServiceModeStartWait()
{
    StartFabricUpgradeManager();
}

void FabricInstallerServiceImpl::ConsoleModeStartWait()
{
    StartFabricUpgradeManager();
}

void FabricInstallerServiceImpl::ServiceModeStopWait()
{
    StopFabricUpgradeManager();
}

void FabricInstallerServiceImpl::ConsoleModeStopWait()
{
    StopFabricUpgradeManager();
}

void FabricInstallerServiceImpl::OnOpenComplete(AsyncOperationSPtr operation)
{
    ErrorCode error = fabricUpgradeManager_->EndOpen(operation);
    Trace.WriteTrace(error.ToLogLevel(),
        TraceType,
        "FabricUpgradeManager open returned {0}",
        error);
    if (!error.IsSuccess())
    {
        Assert::CodingError("FabricUpgradeManager Failed to open with error {0}", error);
    }
}

void FabricInstallerServiceImpl::CreateAndOpen(AutoResetEvent & openEvent)
{
    fabricUpgradeManager_ = make_shared<FabricUpgradeManager>(autoclean_);
    fabricUpgradeManager_->BeginOpen(TimeSpan::FromMinutes(FabricInstallerService::Constants::FabricUpgradeManagerOpenTimeout),
        [&openEvent, this](AsyncOperationSPtr const& operation)
        {
            this->OnOpenComplete(operation);
            openEvent.Set();
        },
        fabricUpgradeManager_->CreateAsyncOperationRoot()
        );
}

void FabricInstallerServiceImpl::StartFabricUpgradeManager()
{
    AutoResetEvent openEvent;

    Threadpool::Post([this, &openEvent]()
    {
        this->CreateAndOpen(openEvent);
    });

    openEvent.WaitOne();
}

void FabricInstallerServiceImpl::OnCloseCompleted(AsyncOperationSPtr operation)
{
    ErrorCode error = fabricUpgradeManager_->EndClose(operation);
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "Close FabricUpgradeManager returned {0}",
        error);
}

void FabricInstallerServiceImpl::Close(AutoResetEvent & closeEvent)
{
    auto timeout = TimeSpan::FromMinutes(FabricInstallerService::Constants::FabricUpgradeManagerCloseTimeout);
    Trace.WriteInfo(
        TraceType,
        "Close FabricUpgradeManager, with timeout {0} ",
        timeout);
    fabricUpgradeManager_->BeginClose(timeout,
        [&closeEvent, this](AsyncOperationSPtr operation)
        {
            this->OnCloseCompleted(operation);
            closeEvent.Set();
        },
        fabricUpgradeManager_->CreateAsyncOperationRoot());
}

void FabricInstallerServiceImpl::StopFabricUpgradeManager()
{
    Trace.WriteInfo(
        TraceType,
        "Stop FabricUpgradeManager called");
    AutoResetEvent closeEvent;
    Threadpool::Post([this, &closeEvent]()
    {
        this->Close(closeEvent);
    });
    closeEvent.WaitOne();
}
