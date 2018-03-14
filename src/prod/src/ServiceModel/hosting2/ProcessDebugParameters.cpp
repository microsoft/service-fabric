// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ProcessDebugParameters::ProcessDebugParameters()
    : exePath_(),
    arguments_(),
    lockFile_(),
    workingFolder_(),
    debugParametersFile_(),
    environmentBlock_(),
    containerEntryPoints_(),
    containerEnvironmentBlock_(),
    containerMountedVolumes_()
{
}

ProcessDebugParameters::ProcessDebugParameters(
    std::wstring const & exePath,
    std::wstring const & arguments,
    std::wstring const & lockFile,
    std::wstring const & workingFolder,
    std::wstring const & debugParametersFile,
    EnvironmentMap const & envVars,
    vector<wstring> const & containerEntryPoints,
    vector<wstring> const & containerMountedVolumes,
    vector<wstring> const & containerEnvironmentBlock)
    : exePath_(exePath),
    arguments_(arguments),
    lockFile_(lockFile),
    workingFolder_(workingFolder),
    debugParametersFile_(debugParametersFile),
    environmentBlock_(),
    containerEntryPoints_(containerEntryPoints),
    containerMountedVolumes_(containerMountedVolumes),
    containerEnvironmentBlock_(containerEnvironmentBlock)
{
    Environment::ToEnvironmentBlock(envVars, environmentBlock_);
}

ProcessDebugParameters::ProcessDebugParameters(ProcessDebugParameters const & other)
    : exePath_(other.exePath_),
    arguments_(other.arguments_),
    lockFile_(other.lockFile_),
    workingFolder_(other.workingFolder_),
    debugParametersFile_(other.debugParametersFile_),
    environmentBlock_(other.environmentBlock_),
    containerEntryPoints_(other.containerEntryPoints_),
    containerMountedVolumes_(other.containerMountedVolumes_),
    containerEnvironmentBlock_(other.containerEnvironmentBlock_)
{
}

ProcessDebugParameters::ProcessDebugParameters(ProcessDebugParameters && other)
    : exePath_(move(other.exePath_)),
    arguments_(move(other.arguments_)),
    lockFile_(move(other.lockFile_)),
    workingFolder_(move(other.workingFolder_)),
    debugParametersFile_(move(other.debugParametersFile_)),
    environmentBlock_(move(other.environmentBlock_)),
    containerEntryPoints_(move(other.containerEntryPoints_)),
    containerMountedVolumes_(move(other.containerMountedVolumes_)),
    containerEnvironmentBlock_(move(other.containerEnvironmentBlock_))
{
}

ProcessDebugParameters const & ProcessDebugParameters::operator=(ProcessDebugParameters const & other)
{
    if(this != &other)
    {
        this->exePath_ = other.exePath_;
        this->arguments_ = other.arguments_;
        this->lockFile_ = other.lockFile_;
        this->workingFolder_ = other.workingFolder_;
        this->debugParametersFile_ = other.debugParametersFile_;
        this->environmentBlock_ = other.environmentBlock_;
        this->containerEntryPoints_ = other.containerEntryPoints_;
        this->containerMountedVolumes_ = other.containerMountedVolumes_;
        this->containerEnvironmentBlock_ = other.containerEnvironmentBlock_;
    }
    return *this;
}

ProcessDebugParameters const & ProcessDebugParameters::operator=(ProcessDebugParameters && other)
{
    if(this != &other)
    {
        this->exePath_ = move(other.exePath_);
        this->arguments_ = move(other.arguments_);
        this->lockFile_ = move(other.lockFile_);
        this->workingFolder_ = move(other.workingFolder_);
        this->debugParametersFile_ = move(other.debugParametersFile_);
        this->environmentBlock_ = move(other.environmentBlock_);
        this->containerEntryPoints_ = move(other.containerEntryPoints_);
        this->containerEnvironmentBlock_ = move(other.containerEnvironmentBlock_);
        this->containerMountedVolumes_ = move(other.containerMountedVolumes_);
    }
    return *this;
}

void ProcessDebugParameters::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ProcessDebugParameters { ");
    w.Write("ExePath = {0}, ", ExePath);
    w.Write("Arguments = {0}, ", Arguments);
    w.Write("LockFile = {0}, ", LockFile);
    w.Write("WorkingFolder = {0}, ", WorkingFolder);
    w.Write("DebugParametersFile = {0}, ", DebugParametersFile);
    w.Write("EnvironmentBlock = {");
    for(auto iter = EnvironmentBlock.cbegin(); iter != EnvironmentBlock.cend(); ++ iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("ContainerEntryPoints = {");
    for (auto iter = ContainerEntryPoints.cbegin(); iter != ContainerEntryPoints.cend(); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");

    w.Write("ContainerMountedVolumes = {");
    for (auto iter = ContainerMountedVolumes.cbegin(); iter != ContainerMountedVolumes.cend(); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");

    w.Write("ContainerEnvironmentBlock = {");
    for (auto iter = ContainerEnvironmentBlock.cbegin(); iter != ContainerEnvironmentBlock.cend(); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");
    w.Write("}");
}

ErrorCode ProcessDebugParameters::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_PROCESS_DEBUG_PARAMETERS & debugParameters) const
{
    debugParameters.ExePath = heap.AddString(this->ExePath);
    debugParameters.Arguments = heap.AddString(this->Arguments);
    debugParameters.LockFile = heap.AddString(this->LockFile);
    debugParameters.WorkingFolder = heap.AddString(this->WorkingFolder);
    debugParameters.DebugParametersFile = heap.AddString(this->DebugParametersFile);

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
    debugParameters.EnvVars = envVars.GetRawPointer();

    auto entryPoints = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, this->ContainerEntryPoints, entryPoints);
    debugParameters.ContainerEntryPoints = entryPoints.GetRawPointer();

    auto mountedVolumes = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, this->ContainerMountedVolumes, mountedVolumes);
    debugParameters.ContainerMountedVolumes = mountedVolumes.GetRawPointer();

    auto containerEnvBlock = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, this->ContainerEnvironmentBlock, containerEnvBlock);
    debugParameters.ContainerEnvironmentBlock = containerEnvBlock.GetRawPointer();

    debugParameters.Reserved = nullptr;

    return ErrorCode::Success();
}

Common::WStringLiteral const ProcessDebugParametersJson::ExePathParameter(L"ExePath");
Common::WStringLiteral const ProcessDebugParametersJson::ArgumentsParameter(L"Arguments");

ProcessDebugParametersJson::ProcessDebugParametersJson() : ExePath(), Arguments()
{
}

ProcessDebugParametersJson::~ProcessDebugParametersJson()
{
}
