// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ProcessDescription::ProcessDescription()
    : exePath_(),
    arguments_(),
    startInDirectory_(),
    environmentBlock_(),
    appDirectory_(),
    tempDirectory_(),
    workDirectory_(),
    logDirectory_(),
    showNoWindow_(),
    redirectConsole_(),
    allowChildProcessDetach_(),
    notAttachedToJob_(),
    redirectedConsoleFileNamePrefix_(),
    consoleRedirectionFileRetentionCount_(),
    consoleRedirectionFileMaxSizeInKb_(),
    debugParameters_(),
    resourceGovernancePolicy_(),
    spResourceGovernance_(),
    cgroupName_()
{
}

ProcessDescription::ProcessDescription(
    std::wstring const & exePath,
    std::wstring const & arguments,
    std::wstring const & startInDirectory,
    EnvironmentMap const & envVars,
    std::wstring const & appDirectory,
    std::wstring const & tempDirectory,
    std::wstring const & workDirectory,
    std::wstring const & logDirectory,
    bool showNoWindow,
    bool redirectConsole,
    bool allowChildProcessDetach, 
    bool notAttachedToJob,
    std::wstring const & redirectedConsoleFileNamePrefix,
    LONG consoleRedirectionFileRetentionCount,
    LONG consoleRedirectionFileMaxSizeInKb,
    ProcessDebugParameters const & debugParameters,
    ResourceGovernancePolicyDescription const & resourceGovernancePolicy,
    ServicePackageResourceGovernanceDescription const & spResourceGovernance,
    std::wstring const & cgroupName)
    : exePath_(exePath),
    arguments_(arguments),
    startInDirectory_(startInDirectory),
    environmentBlock_(),
    envVars_(envVars),
    appDirectory_(appDirectory),
    tempDirectory_(tempDirectory),
    workDirectory_(workDirectory),
    logDirectory_(logDirectory),
    showNoWindow_(showNoWindow),
    redirectConsole_(redirectConsole),
    allowChildProcessDetach_(allowChildProcessDetach),
    notAttachedToJob_(notAttachedToJob),
    redirectedConsoleFileNamePrefix_(redirectedConsoleFileNamePrefix),
    consoleRedirectionFileRetentionCount_(consoleRedirectionFileRetentionCount),
    consoleRedirectionFileMaxSizeInKb_(consoleRedirectionFileMaxSizeInKb),
    debugParameters_(debugParameters),
    resourceGovernancePolicy_(resourceGovernancePolicy),
    spResourceGovernance_(spResourceGovernance),
    cgroupName_(cgroupName)
{
    ASSERT_IF(
        redirectConsole && (redirectedConsoleFileNamePrefix == L"" || consoleRedirectionFileRetentionCount <=0 || consoleRedirectionFileMaxSizeInKb <=0),
        "Invalid value for redirectedConsoleFileNamePrefix or consoleRedirectionFileRetentionCount or consoleRedirectionFileMaxSizeInKb when console redirection is enabled");
    Environment::ToEnvironmentBlock(envVars_, environmentBlock_);
}

ProcessDescription::ProcessDescription(ProcessDescription const & other)
    : exePath_(other.exePath_),
    arguments_(other.arguments_),
    startInDirectory_(other.startInDirectory_),
    environmentBlock_(other.environmentBlock_),
    appDirectory_(other.appDirectory_),
    tempDirectory_(other.tempDirectory_),
    workDirectory_(other.workDirectory_),
    logDirectory_(other.logDirectory_),
    redirectConsole_(other.redirectConsole_),
    allowChildProcessDetach_(other.allowChildProcessDetach_),
    notAttachedToJob_(other.notAttachedToJob_),
    redirectedConsoleFileNamePrefix_(move(other.redirectedConsoleFileNamePrefix_)),
    consoleRedirectionFileRetentionCount_(other.consoleRedirectionFileRetentionCount_),
    consoleRedirectionFileMaxSizeInKb_(other.consoleRedirectionFileMaxSizeInKb_),
    showNoWindow_(other.showNoWindow_),
    debugParameters_(other.debugParameters_),
    resourceGovernancePolicy_(other.resourceGovernancePolicy_),
    spResourceGovernance_(other.spResourceGovernance_),
    cgroupName_(other.cgroupName_)
{
}

ProcessDescription::ProcessDescription(ProcessDescription && other)
    : exePath_(move(other.exePath_)),
    arguments_(move(other.arguments_)),
    startInDirectory_(move(other.startInDirectory_)),
    environmentBlock_(move(other.environmentBlock_)),
    appDirectory_(move(other.appDirectory_)),
    tempDirectory_(move(other.tempDirectory_)),
    workDirectory_(move(other.workDirectory_)),
    logDirectory_(move(other.logDirectory_)),
    redirectConsole_(other.redirectConsole_),
    allowChildProcessDetach_(other.allowChildProcessDetach_),
    notAttachedToJob_(other.notAttachedToJob_),
    redirectedConsoleFileNamePrefix_(move(other.redirectedConsoleFileNamePrefix_)),    
    consoleRedirectionFileRetentionCount_(other.consoleRedirectionFileRetentionCount_),
    consoleRedirectionFileMaxSizeInKb_(other.consoleRedirectionFileMaxSizeInKb_),
    showNoWindow_(other.showNoWindow_),
    debugParameters_(move(other.debugParameters_)),
    resourceGovernancePolicy_(move(other.resourceGovernancePolicy_)),
    spResourceGovernance_(move(other.spResourceGovernance_)),
    cgroupName_(move(other.cgroupName_))
{
}

ProcessDescription const & ProcessDescription::operator=(ProcessDescription const & other)
{
    if(this != &other)
    {
        this->exePath_ = other.exePath_;
        this->arguments_ = other.arguments_;
        this->startInDirectory_ = other.startInDirectory_;
        this->environmentBlock_ = other.environmentBlock_;
        this->appDirectory_ = other.appDirectory_;
        this->tempDirectory_ = other.tempDirectory_;
        this->workDirectory_ = other.workDirectory_;
        this->logDirectory_ = other.logDirectory_;
        this->redirectConsole_ = other.redirectConsole_;
        this->allowChildProcessDetach_ = other.allowChildProcessDetach_;
        this->notAttachedToJob_ = other.notAttachedToJob_;
        this->redirectedConsoleFileNamePrefix_ = other.redirectedConsoleFileNamePrefix_;
        this->consoleRedirectionFileRetentionCount_ = other.consoleRedirectionFileRetentionCount_;
        this->consoleRedirectionFileMaxSizeInKb_ = other.consoleRedirectionFileMaxSizeInKb_;
        this->showNoWindow_ = other.showNoWindow_;
        this->debugParameters_ = other.debugParameters_;
        this->resourceGovernancePolicy_ = other.resourceGovernancePolicy_;
        this->spResourceGovernance_ = other.spResourceGovernance_;
        this->cgroupName_ = other.cgroupName_;
    }
    return *this;
}

ProcessDescription const & ProcessDescription::operator=(ProcessDescription && other)
{
    if(this != &other)
    {
        this->exePath_ = move(other.exePath_);
        this->arguments_ = move(other.arguments_);
        this->startInDirectory_ = move(other.startInDirectory_);
        this->environmentBlock_ = move(other.environmentBlock_);
        this->appDirectory_ = move(other.appDirectory_);
        this->tempDirectory_ = move(other.tempDirectory_);
        this->workDirectory_ = move(other.workDirectory_);
        this->logDirectory_ = move(other.logDirectory_);
        this->redirectConsole_ = other.redirectConsole_;
        this->allowChildProcessDetach_ = other.allowChildProcessDetach_;
        this->notAttachedToJob_ = other.notAttachedToJob_;
        this->redirectedConsoleFileNamePrefix_ = move(other.redirectedConsoleFileNamePrefix_);
        this->consoleRedirectionFileRetentionCount_ = other.consoleRedirectionFileRetentionCount_;
        this->consoleRedirectionFileMaxSizeInKb_ = other.consoleRedirectionFileMaxSizeInKb_;
        this->showNoWindow_ = other.showNoWindow_;
        this->debugParameters_ = move(other.debugParameters_);
        this->resourceGovernancePolicy_ = move(other.resourceGovernancePolicy_);
        this->spResourceGovernance_ = move(other.spResourceGovernance_);
        this->cgroupName_ = move(other.cgroupName_);
    }
    return *this;
}

void ProcessDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ProcessDescription { ");
    w.Write("ExePath = {0}, ", ExePath);
    w.Write("Arguments = {0}, ", Arguments);
    w.Write("EnvironmentBlock = {");
    for(auto iter = EnvironmentBlock.cbegin(); iter != EnvironmentBlock.cend(); ++ iter)
    {
        w.Write("{0}", *iter);
    }
    
    w.Write("}");
    w.Write("StartInDirectory = {0}, ", StartInDirectory);
    w.Write("AppDirectory = {0}, ", AppDirectory);
    w.Write("TempDirectory = {0}, ", TempDirectory);
    w.Write("WorkDirectory = {0}, ", WorkDirectory);
    w.Write("LogDirectory = {0}, ", LogDirectory);
    w.Write("ShowNoWindow = {0}, ", ShowNoWindow);
    w.Write("RedirectConsole = {0}, ", RedirectConsole);
    w.Write("AllowChildProcessDetach = {0}", AllowChildProcessDetach);
    w.Write("NotAttachedToJob = {0}", NotAttachedToJob);
    w.Write("RedirectedConsoleFileNamePrefix = {0}, ", RedirectedConsoleFileNamePrefix);    
    w.Write("ConsoleRedirectionFileRetentionCount = {0}, ", ConsoleRedirectionFileRetentionCount);
    w.Write("ConsoleRedirectionFileMaxSizeInKb = {0}, ", ConsoleRedirectionFileMaxSizeInKb);
    w.Write("Debug Parameters = {0}", debugParameters_);
    w.Write("ResourceGovernancePolicy = {0}", resourceGovernancePolicy_);
    w.Write("ServicePackageResourceGovernance = {0}", spResourceGovernance_);
    w.Write("Cgroup name = {0}", cgroupName_);
    w.Write("}");
}

ErrorCode ProcessDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PROCESS_DESCRIPTION & fabricProcessDesc) const
{
    fabricProcessDesc.ExePath = heap.AddString(this->ExePath);
    fabricProcessDesc.Arguments = heap.AddString(this->Arguments);
    fabricProcessDesc.StartInDirectory = heap.AddString(this->StartInDirectory);

    EnvironmentMap envMap;
    if (!(this->EnvironmentBlock.empty()))
    {
        auto envBlock = this->EnvironmentBlock;
        Environment::FromEnvironmentBlock(envBlock.data(), envMap);
    }

    auto envVars = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    auto error = PublicApiHelper::ToPublicApiStringPairList(heap, envMap, *envVars);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricProcessDesc.EnvVars = envVars.GetRawPointer();

    fabricProcessDesc.AppDirectory = heap.AddString(this->AppDirectory);
    fabricProcessDesc.TempDirectory = heap.AddString(this->TempDirectory);
    fabricProcessDesc.WorkDirectory = heap.AddString(this->WorkDirectory);
    fabricProcessDesc.LogDirectory = heap.AddString(this->LogDirectory);
    fabricProcessDesc.RedirectConsole = this->RedirectConsole;
    fabricProcessDesc.RedirectedConsoleFileNamePrefix = heap.AddString(this->RedirectedConsoleFileNamePrefix);
    fabricProcessDesc.ConsoleRedirectionFileRetentionCount = this->ConsoleRedirectionFileRetentionCount;
    fabricProcessDesc.ConsoleRedirectionFileMaxSizeInKb = this->ConsoleRedirectionFileMaxSizeInKb;
    fabricProcessDesc.ShowNoWindow = this->ShowNoWindow;
    fabricProcessDesc.AllowChildProcessDetach = this->AllowChildProcessDetach;
    fabricProcessDesc.NotAttachedToJob = this->NotAttachedToJob;

    auto debugParams = heap.AddItem<FABRIC_PROCESS_DEBUG_PARAMETERS>();
    error = this->DebugParameters.ToPublicApi(heap, *debugParams);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricProcessDesc.DebugParameters = debugParams.GetRawPointer();

    auto resGovPolicy = heap.AddItem<FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION>();
    error = this->ResourceGovernancePolicy.ToPublicApi(heap, *resGovPolicy);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricProcessDesc.ResourceGovernancePolicy = resGovPolicy.GetRawPointer();

    auto spResGov = heap.AddItem<FABRIC_SERVICE_PACKAGE_RESOURCE_GOVERNANCE_DESCRIPTION>();
    error = this->ServicePackageResourceGovernance.ToPublicApi(heap, *spResGov);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricProcessDesc.ServicePackageResourceGovernance = spResGov.GetRawPointer();

    fabricProcessDesc.CgroupName = heap.AddString(this->CgroupName);
    fabricProcessDesc.Reserved = nullptr;

    return ErrorCode(ErrorCodeValue::Success);
}
