// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;

StringLiteral const TraceEtwSessionProvider("EtwSessionProvider");
const std::wstring EtwSessionProvider::TraceSessionNamePrefix = L"FabricApplicationTraces_";
const std::wstring EtwSessionProvider::TracesFolderName = L"Traces";
const std::wstring EtwSessionProvider::LogFileNamePrefix = L"fabric_application_traces_";
const std::wstring EtwSessionProvider::LogFileNameSuffix = L"_%d.etl";

EtwSessionProvider::EtwSessionProvider(ComponentRoot const & root, wstring const & nodeName, wstring const & deploymentFolder)
    : RootedObject(root),
    FabricComponent(),
    traceSessionHandle_(0),
    etwGuidMap_(),
    etwGuidMapLock_(),
    etwGuidMapIsClosed_(false)
{
    traceSessionName_ = TraceSessionNamePrefix;
    traceSessionName_.append(nodeName);

    tracePath_ = Path::Combine(deploymentFolder, TracesFolderName);
}

EtwSessionProvider::~EtwSessionProvider()
{
}

ErrorCode EtwSessionProvider::SetupServiceEtw(
    ServiceModel::ServicePackageDescription const & servicePackageDescription,
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteError(
            TraceEtwSessionProvider,
            "Failed to enable ETW trace collection for service {0} because the EtwSessionProvider is not opened.", 
            packageEnvironmentContext->ServicePackageInstanceId);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    ErrorCode error;
    for (auto iter = servicePackageDescription.Diagnostics.EtwDescription.ProviderGuids.begin(); 
         iter != servicePackageDescription.Diagnostics.EtwDescription.ProviderGuids.end(); 
         ++iter)
    {
        error = EnableGuidForTraceSession(*iter);
        if (!error.IsSuccess())
        {
            return error;
        }
        packageEnvironmentContext->AddEtwProviderGuid(*iter);
    }
    return error;
}

void EtwSessionProvider::CleanupServiceEtw(
    ServicePackageInstanceEnvironmentContextSPtr  const & packageEnvironmentContext)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteError(
            TraceEtwSessionProvider,
            "Failed to disable ETW trace collection for service {0} because the EtwSessionProvider is not opened.", 
            packageEnvironmentContext->ServicePackageInstanceId);
        return;
    }

    for (auto iter = packageEnvironmentContext->EtwProviderGuids.begin(); 
         iter != packageEnvironmentContext->EtwProviderGuids.end(); 
         ++iter)
    {
        DisableGuidForTraceSession(*iter, true);
    }
}

// ********************************************************************************************************************
// FabricComponent methods

ErrorCode EtwSessionProvider::OnOpen()
{
    auto error = StopEtwTraceSession();
    if (!error.IsSuccess())
    {
        return error;
    }
    etwGuidMapIsClosed_ = false;

    return ErrorCode::Success();
}

ErrorCode EtwSessionProvider::OnClose()
{
     return StopEtwTraceSession();
}

void EtwSessionProvider::OnAbort()
{
    StopEtwTraceSession();
}

ErrorCode EtwSessionProvider::StartEtwTraceSession()
{
    ErrorCode error = Directory::Create2(tracePath_);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceEtwSessionProvider,
            "Failed to create directory {0}, error {1}",
            tracePath_,
            error);
        return error;
    }

    wstring logFileName = wformatString("{0}{1}{2}", LogFileNamePrefix, DateTime::Now().Ticks, LogFileNameSuffix);
    wstring logFileFullName = Path::Combine(tracePath_, logFileName);

    ULONG propertiesBufferSize;
    error = ComputePropertiesBufferSize(logFileFullName, propertiesBufferSize);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceEtwSessionProvider,
            "Unable to compute size of EVENT_TRACE_PROPERTIES structure to start trace session. Log file name: {0}, trace session name: {1}, error {2}",
            logFileName,
            traceSessionName_,
            error);
        return error;
    }

    ByteBuffer buffer;
    buffer.resize(propertiesBufferSize, 0);
    PEVENT_TRACE_PROPERTIES eventTraceProperties = GetEventTraceProperties(buffer);
    eventTraceProperties->Wnode.BufferSize = propertiesBufferSize;
    eventTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    eventTraceProperties->FlushTimer = DefaultTraceSessionFlushIntervalInSeconds;
    eventTraceProperties->MaximumFileSize = HostingConfig::GetConfig().ApplicationEtwTraceFileSizeInMB;
    eventTraceProperties->LogFileMode = EVENT_TRACE_FILE_MODE_NEWFILE;
    eventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (((ULONG) traceSessionName_.length()) * sizeof(WCHAR)) + sizeof(WCHAR);
    eventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);


    memcpy_s(
        ((PBYTE) eventTraceProperties) + eventTraceProperties->LogFileNameOffset,
        propertiesBufferSize - eventTraceProperties->LogFileNameOffset,
        logFileFullName.c_str(),
        (logFileFullName.length() * sizeof(WCHAR)) + sizeof(WCHAR));

    ULONG startTraceResult = StartTrace(&traceSessionHandle_, traceSessionName_.c_str(), eventTraceProperties);
    if (ERROR_SUCCESS != startTraceResult)
    {
        WriteError(
            TraceEtwSessionProvider,
            "Failed to start trace session {0}. Error {1}.",
            traceSessionName_,
            error);
        return ErrorCode::FromWin32Error(startTraceResult);
    }

    WriteInfo(
        TraceEtwSessionProvider,
        "Trace session {0} started. Log files: {1}.",
        traceSessionName_,
        logFileFullName);

    return ErrorCode::Success();
}

void EtwSessionProvider::DisableAllGuidsForTraceSession()
{
    {
        AcquireWriteLock writeLock(etwGuidMapLock_);
        for(auto iter = etwGuidMap_.begin(); iter != etwGuidMap_.end(); ++iter)
        {
            DisableGuidForTraceSessionLocked(iter, false);
        }
        etwGuidMap_.clear();
        etwGuidMapIsClosed_ = true;
    }
}

ErrorCode EtwSessionProvider::StopEtwTraceSession()
{
    DisableAllGuidsForTraceSession();

    ULONG propertiesBufferSize;
    ErrorCode error = ComputePropertiesBufferSize(wstring(L""), propertiesBufferSize);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceEtwSessionProvider,
            "Unable to compute size of EVENT_TRACE_PROPERTIES structure to stop trace session {0}. Error {1}.",
            traceSessionName_,
            error);
        return error;
    }

    ByteBuffer buffer;
    buffer.resize(propertiesBufferSize, 0);
    PEVENT_TRACE_PROPERTIES eventTraceProperties = GetEventTraceProperties(buffer);
    eventTraceProperties->Wnode.BufferSize = propertiesBufferSize;
    eventTraceProperties->LogFileNameOffset = 0;
    eventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    ULONG stopTraceResult = ControlTrace(
        NULL,
        traceSessionName_.c_str(),
        eventTraceProperties,
        EVENT_TRACE_CONTROL_STOP);
    // MSDN says the following about the ERROR_MORE_DATA return code from this API:
    // "The buffer for EVENT_TRACE_PROPERTIES is too small to hold all the information 
    // for the session. If you do not need the session's property information, you can
    // ignore this error. If you receive this error when stopping the session, ETW will
    // have already stopped the session before generating this error."
    if ((ERROR_SUCCESS != stopTraceResult) && (ERROR_MORE_DATA != stopTraceResult))
    {
        if (ERROR_WMI_INSTANCE_NOT_FOUND == stopTraceResult)
        {
            WriteInfo(
                TraceEtwSessionProvider,
                "Trace session {0} is not running, so there is no need to stop it.",
                traceSessionName_);
        }
        else
        {
            WriteError(
                TraceEtwSessionProvider,
                "Failed to stop trace session {0}. Error {1}.",
                traceSessionName_,
                error);
            return ErrorCode::FromWin32Error(stopTraceResult);
        }
    }
    else
    {
        WriteInfo(
            TraceEtwSessionProvider,
            "Trace session {0} stopped.",
            traceSessionName_);
    }

    return ErrorCode::Success();
}

ErrorCode EtwSessionProvider::ComputePropertiesBufferSize(
    wstring const & logFileName,
    __out ULONG & propertiesBufferSize)
{
    propertiesBufferSize = 0;

    HRESULT hr;
    ULONG sizeIncrement;

    if (!logFileName.empty())
    {
        // Compute log file name size
        hr = ULongMult((ULONG)logFileName.length(), sizeof(WCHAR), &sizeIncrement);
        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
        // Add terminating NULL
        hr = ULongAdd(sizeIncrement, sizeof(WCHAR), &sizeIncrement);
        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }

        // Add log file name size to total size
        hr = ULongAdd(propertiesBufferSize, sizeIncrement, &propertiesBufferSize);
        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
    }

    // Compute trace session name size
    hr = ULongMult((ULONG) traceSessionName_.length(), sizeof(WCHAR), &sizeIncrement);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }
    // Add terminating NULL
    hr = ULongAdd(sizeIncrement, sizeof(WCHAR), &sizeIncrement);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    // Add session name size to total size
    hr = ULongAdd(propertiesBufferSize, sizeIncrement, &propertiesBufferSize);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    // Add size of EVENT_TRACE_PROPERTIES structure to total size
    hr = ULongAdd(propertiesBufferSize, sizeof(EVENT_TRACE_PROPERTIES), &propertiesBufferSize);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    return ErrorCode::Success();
}

PEVENT_TRACE_PROPERTIES EtwSessionProvider::GetEventTraceProperties(
    ByteBuffer const & buffer)
{
    if (buffer.size() == 0)
    {
        return NULL;
    }
    else
    {
        auto pByte = const_cast<BYTE*>(buffer.data());
        return reinterpret_cast<PEVENT_TRACE_PROPERTIES>(pByte);
    }
}

ErrorCode EtwSessionProvider::EnableGuidForTraceSession(wstring const & providerGuid)
{
    {
        AcquireWriteLock writeLock(etwGuidMapLock_);

        if (etwGuidMapIsClosed_)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        if (0 == traceSessionHandle_)
        {
            ErrorCode startTraceError = StartEtwTraceSession();
            if (!startTraceError.IsSuccess())
            {
                return startTraceError;
            }
        }

        pair<wstring, int> refCountToInsert;
        refCountToInsert.first = providerGuid;
        refCountToInsert.second = 1;

        auto insertResult = etwGuidMap_.insert(refCountToInsert);
        if (insertResult.second)
        {
            UCHAR traceLevel = (UCHAR) HostingConfig::GetConfig().ApplicationEtwTraceLevel;
            if (traceLevel < TRACE_LEVEL_CRITICAL)
            {
                traceLevel = TRACE_LEVEL_CRITICAL;
            }
            else if (traceLevel > TRACE_LEVEL_VERBOSE)
            {
                traceLevel = TRACE_LEVEL_VERBOSE;
            }

            Common::Guid guid(providerGuid);
            ULONG error = EnableTraceEx(
                &guid.AsGUID(),
                NULL,
                traceSessionHandle_,
                1,
                traceLevel,
                0,
                0,
                0,
                NULL);
            if (ERROR_SUCCESS != error)
            {
                WriteError(
                    TraceEtwSessionProvider,
                    "Failed to enable provider {0} for trace session {1}. Error {2}.",
                    providerGuid,
                    traceSessionName_,
                    error);
                etwGuidMap_.erase(insertResult.first);
                return ErrorCode::FromWin32Error(error);
            }
            else
            {
                WriteInfo(
                    TraceEtwSessionProvider,
                    "Provider {0} enabled for trace session {1}.",
                    providerGuid,
                    traceSessionName_);
            }
        }
        else
        {
            ((*(insertResult.first)).second)++;
            WriteInfo(
                TraceEtwSessionProvider,
                "Provider {0} was already enabled for trace session {1} on behalf of other services.",
                providerGuid,
                traceSessionName_);
        }
    }

    return ErrorCode::Success();
}

ErrorCode EtwSessionProvider::DisableGuidForTraceSession(std::wstring const & providerGuid, bool removeGuidFromMap)
{
    {
        AcquireWriteLock writeLock(etwGuidMapLock_);

        if (etwGuidMapIsClosed_)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        DisableGuidForTraceSessionLocked(providerGuid, removeGuidFromMap);
    }
    return ErrorCode::Success();
}

ErrorCode EtwSessionProvider::DisableGuidForTraceSessionLocked(std::wstring const & providerGuid, bool removeGuidFromMap)
{
    auto findResult = etwGuidMap_.find(providerGuid);
    if (findResult != etwGuidMap_.end())
    {
        return DisableGuidForTraceSessionLocked(findResult, removeGuidFromMap);
    }
    return ErrorCode::Success();
}

ErrorCode EtwSessionProvider::DisableGuidForTraceSessionLocked(std::map<std::wstring, int>::iterator & findResult, bool removeGuidFromMap)
{
    wstring providerGuid = (*findResult).first;

    ((*findResult).second)--;
    if (0 == (*findResult).second)
    {
        if (removeGuidFromMap)
        {
            etwGuidMap_.erase(findResult);
        }

        Common::Guid guid(providerGuid);
        ULONG error = EnableTraceEx(
            &guid.AsGUID(),
            NULL,
            traceSessionHandle_,
            0,
            TRACE_LEVEL_CRITICAL,
            0,
            0,
            0,
            NULL);
        if (ERROR_SUCCESS != error)
        {
            WriteError(
                TraceEtwSessionProvider,
                "Failed to disable provider {0} for trace session {1}. Error {2}.",
                providerGuid,
                traceSessionName_,
                error);
            return ErrorCode::FromWin32Error(error);
        }
        else
        {
            WriteInfo(
                TraceEtwSessionProvider,
                "Provider {0} disabled for trace session {1}.",
                providerGuid,
                traceSessionName_);
        }
    }
    else
    {
        WriteInfo(
            TraceEtwSessionProvider,
            "Provider {0} is still enabled for trace session {1} because other services need it.",
            providerGuid,
            traceSessionName_);
    }
    return ErrorCode::Success();
}
