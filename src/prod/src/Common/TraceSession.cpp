// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/TraceSession.h"

#ifndef PLATFORM_UNIX
#include "VersionHelpers.h"
#include "Management/ImageModel/FabricDeploymentSpecification.h"
#endif

using namespace std;

namespace Common
{
#ifndef PLATFORM_UNIX
    GlobalWString TraceSession::FabricTraceFileName = make_global<wstring>(L"fabric_traces");
    GlobalWString TraceSession::LeaseTraceFileName = make_global<wstring>(L"lease_traces");
    GlobalWString TraceSession::SFBDMiniportTraceFileName = make_global<wstring>(L"SFBDMiniport_traces");
    GlobalWString TraceSession::AppInfoTraceFileName = make_global<wstring>(L"appinfo_traces");
    GlobalWString TraceSession::AppInfoTraceFolderName = make_global<wstring>(L"AppInstanceData\\Etl");
    GlobalWString TraceSession::QueryTraceFileName = make_global<wstring>(L"query_traces");
    GlobalWString TraceSession::QueryTraceFolderName = make_global<wstring>(L"QueryTraces");
    GlobalWString TraceSession::OperationalTraceFileName = make_global<wstring>(L"operational_traces");
    GlobalWString TraceSession::OperationalTraceFolderName = make_global<wstring>(L"OperationalTraces");

    GlobalWString TraceSession::LeaseTraceSessionName = make_global<wstring>(L"FabricLeaseLayerTraces");
    GlobalWString TraceSession::SFBDMiniportTraceSessionName = make_global<wstring>(L"FabricSFBDMiniportTraces");
    GlobalWString TraceSession::FabricTraceSessionName = make_global<wstring>(L"FabricTraces");
    GlobalWString TraceSession::AppInfoTraceSessionName = make_global<wstring>(L"FabricAppInfoTraces");
    GlobalWString TraceSession::QueryTraceSessionName = make_global<wstring>(L"FabricQueryTraces");
    GlobalWString TraceSession::FabricCountersSessionName = make_global<wstring>(L"FabricCounters");
    GlobalWString TraceSession::OperationalTraceSessionName = make_global<wstring>(L"FabricOperationalTraces");

    GlobalWString TraceSession::FabricDataRoot = make_global<wstring>(Environment::Expand(L"%ProgramData%\\Microsoft Service Fabric"));

    Common::Global<Common::Guid> TraceSession::LeaseDriverGuid = make_global<Common::Guid>(L"3f68b79e-a1cf-4b10-8cfd-3dfe322f07cb");
    Common::Global<Common::Guid> TraceSession::SFBDMiniportDriverGuid = make_global<Common::Guid>(L"dfd1a88f-801e-45ce-8f66-44001cedd671");
    Common::Global<Common::Guid> TraceSession::KTLGuid = make_global<Common::Guid>(L"E658F859-2416-4AEF-9DFC-4D303897A37A");
    Common::Global<Common::Guid> TraceSession::FabricGuid = make_global<Common::Guid>(L"cbd93bc2-71e5-4566-b3a7-595d8eeca6e8");
    Common::Global<Common::Guid> TraceSession::MSFIGuid = make_global<Common::Guid>(L"fa8ea6ea-9bf3-4630-b3ef-2b01ebb2a69b");
    Common::Global<Common::Guid> TraceSession::ProgrammingModel_ActorGuid = make_global<Common::Guid>(L"e2f2656b-985e-5c5b-5ba3-bbe8a851e1d7");
    Common::Global<Common::Guid> TraceSession::ProgrammingModel_ServicesGuid = make_global<Common::Guid>(L"27b7a543-7280-5c2a-b053-f2f798e2cbb7");
    Common::Global<Common::Guid> TraceSession::SF_ClientLib_HttpGuid = make_global<Common::Guid>(L"c4766d6f-5414-5d26-48de-873499ff0976");
    Common::Global<Common::Guid> TraceSession::SFPOSCoordinatorServiceGuid = make_global<Common::Guid>(L"24afa313-0d3b-4c7c-b485-1047fd964b60");
    Common::Global<Common::Guid> TraceSession::SFPOSNodeAgentServiceGuid = make_global<Common::Guid>(L"e39b723c-590c-4090-abb0-11e3e6616346");
    Common::Global<Common::Guid> TraceSession::SFPOSNodeAgentNTServiceGuid = make_global<Common::Guid>(L"fc0028ff-bfdc-499f-80dc-ed922c52c5e9");
    Common::Global<Common::Guid> TraceSession::SFPOSNodeAgentSFUtilityGuid = make_global<Common::Guid>(L"05dc046c-60e9-4ef7-965e-91660adffa68");
    Common::Global<Common::Guid> TraceSession::ReliableCollectionGuid = make_global<Common::Guid>(L"ecec9d7a-5003-53fa-270b-5c9f9cc66271");
    Common::Global<Common::Guid> TraceSession::AzureFilesVolumeDriverGuid = make_global<Common::Guid>(L"74CF0846-E6A3-4a3e-A10F-80FD527DA5FD");

    GlobalWString Common::TraceSession::CurrentExeVersion = make_global<wstring>(Environment::GetCurrentModuleFileVersion());
#endif
    StringLiteral const TraceType("TraceSession");

    TraceSession::TraceSessionSPtr TraceSession::Instance()
    {
        static TraceSessionSPtr traceSessionSPtr;

        if (traceSessionSPtr)
        {
            return traceSessionSPtr;
        }

        // We cannot use make_shared below as the class constructor is private 
        traceSessionSPtr = shared_ptr<TraceSession>(new TraceSession());
        return traceSessionSPtr;
    }

    TraceSession::TraceSession()
    {
#ifndef PLATFORM_UNIX
        if (ContainerEnvironment::IsContainerHost())
        {
            tracePath_ = ContainerEnvironment::GetContainerTracePath();
        }
        else
        {
            wstring fabricDataRoot;
            ErrorCode result = Common::FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
            if (!result.IsSuccess())
            {
                fabricDataRoot = FabricDataRoot;
            }

            Management::ImageModel::FabricDeploymentSpecification spec(fabricDataRoot);
            wstring fabricLogRoot;
            result = Common::FabricEnvironment::GetFabricLogRoot(fabricLogRoot);
            if (result.IsSuccess())
            {
                spec.LogRoot = fabricLogRoot;
            }

            tracePath_ = spec.GetTracesFolder();
        }

        FabricEnvironment::GetFabricTracesTestKeyword(testKeyword_);
        FabricEnvironment::GetEnableCircularTraceSession(enableCircularTraceSession_);

        ULONGLONG testKeyword = TraceKeywords::ParseTestKeyword(testKeyword_);

        dataCollectorSets.push_back(
        {
            TraceSessionKind::Fabric,
            GetSessionName(TraceSession::FabricTraceSessionName),
            L"",
            TraceSession::FabricTraceFileName,
            {
                TraceSession::FabricGuid,
                TraceSession::MSFIGuid,
                TraceSession::ProgrammingModel_ActorGuid,
                TraceSession::SF_ClientLib_HttpGuid,
                TraceSession::SFPOSCoordinatorServiceGuid,
                TraceSession::SFPOSNodeAgentServiceGuid,
                TraceSession::SFPOSNodeAgentNTServiceGuid,
                TraceSession::SFPOSNodeAgentSFUtilityGuid,
                TraceSession::ProgrammingModel_ServicesGuid,
                TraceSession::ReliableCollectionGuid,
                TraceSession::AzureFilesVolumeDriverGuid
            },
            TRACE_LEVEL_NONE,
            {
                Common::TraceKeywords::Default,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All,
                Common::TraceKeywords::All
            },
            {
                Common::TraceKeywords::Default | testKeyword,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0
            },
            TraceSession::DefaultTraceBufferSizeInKB,
            TraceSession::DefaultTraceFileSizeInMB,
            TraceSession::DefaultTraceFlushIntervalInSeconds
        }
        );

        dataCollectorSets.push_back(
        {
            TraceSessionKind::Lease,
            GetSessionName(TraceSession::LeaseTraceSessionName),
            L"",
            TraceSession::LeaseTraceFileName,
            { TraceSession::LeaseDriverGuid, TraceSession::KTLGuid },
            TRACE_LEVEL_INFORMATION,
            { 0, 0 },
            { 0, 0 },
            TraceSession::DefaultTraceBufferSizeInKB,
            TraceSession::DefaultTraceFileSizeInMB,
            TraceSession::DefaultTraceFlushIntervalInSeconds
        }
        );
        
        //Here checking for windows 10 and only start block store driver traces on windows 10+
        //because containers are only avilable on windows server 10+. Containers are prerequisite for block store.
        if (IsWindows10OrGreater())
        {
            dataCollectorSets.push_back(
                {
                    TraceSessionKind::SFBDMiniport,
                    GetSessionName(TraceSession::SFBDMiniportTraceSessionName),
                    L"",
                    TraceSession::SFBDMiniportTraceFileName,
                { TraceSession::SFBDMiniportDriverGuid },
                TRACE_LEVEL_INFORMATION,
                { 0, 0 },
                { 0, 0 },
                TraceSession::DefaultTraceBufferSizeInKB,
                TraceSession::DefaultTraceFileSizeInMB,
                TraceSession::DefaultTraceFlushIntervalInSeconds
                }
            );
        }
        dataCollectorSets.push_back(
        {
            TraceSessionKind::AppInfo,
            GetSessionName(TraceSession::AppInfoTraceSessionName),
            TraceSession::AppInfoTraceFolderName,
            TraceSession::AppInfoTraceFileName,
            { TraceSession::FabricGuid },
            TRACE_LEVEL_INFORMATION,
            { Common::TraceKeywords::AppInstance },
            { Common::TraceKeywords::AppInstance | testKeyword },
            TraceSession::DefaultAppInfoTraceBufferSizeInKB,
            TraceSession::DefaultTraceFileSizeInMB,
            TraceSession::DefaultAppInfoTraceFlushIntervalInSeconds
        }
        );

        dataCollectorSets.push_back(
        {
            TraceSessionKind::Query,
            GetSessionName(TraceSession::QueryTraceSessionName),
            TraceSession::QueryTraceFolderName,
            TraceSession::QueryTraceFileName,
            { TraceSession::FabricGuid },
            TRACE_LEVEL_INFORMATION,
            { Common::TraceKeywords::ForQuery },
            { Common::TraceKeywords::ForQuery | testKeyword },
            TraceSession::DefaultTraceBufferSizeInKB,
            TraceSession::DefaultTraceFileSizeInMB,
            TraceSession::DefaultTraceFlushIntervalInSeconds
        }
        );


        dataCollectorSets.push_back(
        {
            TraceSessionKind::Operational,
            GetSessionName(TraceSession::OperationalTraceSessionName),
            TraceSession::OperationalTraceFolderName,
            TraceSession::OperationalTraceFileName,
            { TraceSession::FabricGuid },
            TRACE_LEVEL_INFORMATION,
            { Common::TraceKeywords::OperationalChannel },
            { 0 },
            TraceSession::DefaultTraceBufferSizeInKB,
            TraceSession::DefaultTraceFileSizeInMB,
            TraceSession::DefaultTraceFlushIntervalInSeconds
        }
        );
#endif
    }

#ifndef PLATFORM_UNIX
    HRESULT TraceSession::StopTraceSession(DataCollectorSetInfo const & info)
    {
        auto pSessionProperties = GetSessionProperties(info, 0);
        HRESULT result = ControlTrace(NULL, info.SessionName.c_str(), pSessionProperties.get(), EVENT_TRACE_CONTROL_STOP);

        // If the instance cannot be found consider success.
        // If the session properties require more data the session is already stopped and we do not use the session properties anyways.
        if (ERROR_WMI_INSTANCE_NOT_FOUND == result ||
            ERROR_MORE_DATA == result)
        {
            result = S_OK;
        }

        return TraceResult(
            L"StopTraceSession",
            result);
    }

    ErrorCode TraceSession::StopTraceSession(TraceSessionKind::Enum traceSessionKind)
    {
        HRESULT result = ERROR_NOT_FOUND;
        for (auto it = dataCollectorSets.begin(); it != dataCollectorSets.end(); ++it)
        {
            if (it->TraceSessionKind == traceSessionKind)
            {
                result = StopTraceSession(*it);
                break;
            }
        }

        return ErrorCode::FromHResult(TraceResult(L"StopTraceSession", result));
    }
#endif

    ErrorCode TraceSession::StopTraceSessions()
    {
        HRESULT hr = E_FAIL;
#ifndef PLATFORM_UNIX
        for (auto it = dataCollectorSets.begin(); it != dataCollectorSets.end(); ++it)
        {
            hr = StopTraceSession(*it);
            if (ERROR_SUCCESS != hr)
            {
                break;
            }
        }
#else
        string stopTraceSessionCmd = "lttng stop";
        if (0 != system(stopTraceSessionCmd.c_str()))
        {
            hr = E_FAIL;
            WriteError(TraceType, "Unable to stop trace sessions: ErrorCode={0}", ErrorCode::FromHResult(hr));
        }
        else
        {
            hr = S_OK;
            WriteNoise(TraceType, "Trace sessions stopped successfully");
        }
#endif
        return ErrorCode::FromHResult(TraceResult(L"StopTraceSessions", hr));
    }

#ifndef PLATFORM_UNIX
    wstring TraceSession::GetTraceFolder(DataCollectorSetInfo const & info)
    {
        if (!info.TraceFolderName.empty())
        {
            // Trace folder explicitly specified
            wstring logFolder = Path::GetDirectoryName(tracePath_);
            return Path::Combine(logFolder, info.TraceFolderName);
        }
        else
        {
            // Default trace folder
            return tracePath_;
        }
    }

    // additionalBytes allows us to allocate a structure for querying current session status.
    unique_ptr<EVENT_TRACE_PROPERTIES, decltype(free)*> TraceSession::GetSessionProperties(DataCollectorSetInfo const & info, uint additionalBytes)
    {
        wstring traceFolder = GetTraceFolder(info);

        // Add 6 digit serial number to filename
        wstring fileName = wformatString(
            "{0}_{1}_{2}_{3}.etl",
            info.TraceFileName,
            TraceSession::CurrentExeVersion,
            DateTime::Now().Ticks,
            enableCircularTraceSession_ ? "0" : "%d");

        wstring filePath = Path::Combine(traceFolder, fileName);

        size_t propertyBufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(wchar_t) * (info.SessionName.size() + filePath.size() + 2) + additionalBytes;
        auto pSessionProperties = unique_ptr<EVENT_TRACE_PROPERTIES, decltype(free)*>{ (EVENT_TRACE_PROPERTIES*)malloc(propertyBufferSize), free };
        if (NULL == pSessionProperties)
        {
            return pSessionProperties;
        }

        // Reference for below structure : https://msdn.microsoft.com/en-us/library/windows/desktop/aa363784%28v=vs.85%29.aspx

        ZeroMemory(pSessionProperties.get(), propertyBufferSize);
        pSessionProperties->Wnode.BufferSize = static_cast<ULONG>(propertyBufferSize);
        pSessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;    // to indicate that the structure contains event tracing information.
        pSessionProperties->LogFileMode = enableCircularTraceSession_ ? EVENT_TRACE_FILE_MODE_CIRCULAR : EVENT_TRACE_FILE_MODE_NEWFILE | EVENT_TRACE_FILE_MODE_SEQUENTIAL;    //Automatically switches to a new log file when the file reaches the maximum size.
        pSessionProperties->MaximumFileSize = enableCircularTraceSession_ ? info.FileSizeInMB * TraceSession::DefaultCircularFileSizeMultiplier : info.FileSizeInMB;    //Max size of the file used to log events (in MB). Circular is increased 4x.
        pSessionProperties->BufferSize = info.BufferSizeInKB;    //Amount of memory allocated for each event tracing session. Max is 1 MB
        pSessionProperties->FlushTimer = info.FlushIntervalInSeconds;    //How often trace buffers are forcibly flushed
        pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        pSessionProperties->LogFileNameOffset = static_cast<ULONG>(sizeof(EVENT_TRACE_PROPERTIES) + (sizeof(wchar_t) * (info.SessionName.size() + 1)));
        StringCbCopy((LPWSTR)((char*)pSessionProperties.get() + pSessionProperties->LogFileNameOffset), propertyBufferSize - pSessionProperties->LogFileNameOffset, filePath.c_str());
        return pSessionProperties;
    }

    HRESULT TraceSession::StartTraceSession(DataCollectorSetInfo const & info)
    {
        HRESULT hr = E_FAIL;

        // Create ETW sessions for traces.
        TRACEHANDLE sessionHandle = 0;
        auto pSessionProperties = GetSessionProperties(info, 0);
        hr = StartTrace(&sessionHandle, info.SessionName.c_str(), pSessionProperties.get());
        if (ERROR_ALREADY_EXISTS == hr)
        {
            // No more work to do
            return TraceResult(
                L"StartTraceSession",
                ERROR_SUCCESS);
        }

        if (ERROR_SUCCESS != hr)
        {
            return TraceResult(
                L"StartTraceSession",
                hr);
        }

        // Enable providers to write to sessions.
        for (int j = 0; j < _countof(info.ProviderGuids); j++)
        {
            if (!info.ProviderGuids[j].Equals(Guid::Empty()))
            {
                hr = EnableTraceEx2(
                    sessionHandle,
                    &info.ProviderGuids[j].AsGUID(),
                    EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                    info.TraceLevel,
                    info.Keyword[j],
                    info.KeywordAll[j],
                    0,
                    NULL);

                if (ERROR_SUCCESS != hr)
                {
                    break;
                }
            }
        }

        return TraceResult(
            L"StartTraceSession",
            hr);
    }

    ErrorCode TraceSession::StartTraceSession(TraceSessionKind::Enum traceSessionKind)
    {
        HRESULT result = ERROR_NOT_FOUND;

        for (auto it = dataCollectorSets.begin(); it != dataCollectorSets.end(); ++it)
        {
            if (it->TraceSessionKind == traceSessionKind)
            {
                BOOLEAN isRunningLatest;
                HRESULT hr = Prepare(*it, &isRunningLatest);
                if (SUCCEEDED(hr) && !isRunningLatest)
                {
                    result = StartTraceSession(*it);
                }

                break;
            }
        }

        return ErrorCode::FromHResult(TraceResult(L"StartTraceSession", result));
    }
#endif

    /*
    This method will recreate the sessions if they are out-of-date.
    */
    ErrorCode TraceSession::StartTraceSessions()
    {
        HRESULT allSuccessful = ERROR_SUCCESS;
#ifndef PLATFORM_UNIX
        // Before adding the ACLs, create folders for each trace session producing ETLs.
        // If we don't create them explicitly, the PLA API will create them for us when we 
        // create the trace/performance counter session. However, when PLA creates the folder,
        // it does not seem to inherit the ACLs from the parent folder, which is undesirable. 
        // So we create the folder ourselves.
        for (auto it = dataCollectorSets.begin(); it != dataCollectorSets.end(); ++it)
        {
            BOOLEAN isRunningLatest;
            HRESULT hr = Prepare(*it, &isRunningLatest);
            if (!SUCCEEDED(hr))
            {
                allSuccessful = hr;
                continue;
            }

            if (isRunningLatest)
            {
                continue;
            }

            hr = StopTraceSession(*it);
            if (!SUCCEEDED(hr))
            {
                WriteError(TraceType, "Unable to stop trace session {0} status: {1}", it->SessionName, hr);
                allSuccessful = hr;
                continue;
            }

            hr = StartTraceSession(*it);
            if (!SUCCEEDED(hr))
            {
                WriteError(TraceType, "Unable to start trace session {0} status: {1}", it->SessionName, hr);
                allSuccessful = hr;
                continue;
            }
        }
#else
        
        if (ContainerEnvironment::IsContainerHost())
        {
            int linuxTraceDiskBufferSizeInsideContainerInMB = DiagnosticsConfig::GetConfig().LinuxTraceDiskBufferSizeInsideContainerInMB;
            allSuccessful = StartLinuxTraceSessions(linuxTraceDiskBufferSizeInsideContainerInMB, true);            
        }
        else
        {
            int linuxTraceDiskBufferSizeInMB = DiagnosticsConfig::GetConfig().LinuxTraceDiskBufferSizeInMB;
            allSuccessful = StartLinuxTraceSessions(linuxTraceDiskBufferSizeInMB, false); 
        }

        if (!SUCCEEDED(allSuccessful))
        {
            WriteError(TraceType, "Unable to start trace session status: {0}", allSuccessful);
        }
#endif
        return ErrorCode::FromHResult(TraceResult(L"StartTraceSessions", allSuccessful));
    }

#ifndef PLATFORM_UNIX
    HRESULT TraceSession::Prepare(DataCollectorSetInfo const & info, __out BOOLEAN *runningLatestStatus)
    {
        wstring folderName = GetTraceFolder(info);

        ErrorCode errCode = ErrorCodeValue::Success;
        if (!Directory::Exists(folderName))
        {
            errCode = Directory::Create2(folderName);
            if (!errCode.IsSuccess())
            {
                WriteError(TraceType, "Unable to create directory {0} for traces due to error: {1}", folderName, errCode.ErrorCodeValueToString());
                return errCode.ToHResult();
            }
        }

        HRESULT hr = GetTraceSessionStatus(info, runningLatestStatus);
        if (!SUCCEEDED(hr))
        {
            WriteError(TraceType, "Unable to query trace session {0} status: {1}", info.SessionName, hr);
        }

        return hr;
    }

    HRESULT TraceSession::GetTraceSessionStatus(DataCollectorSetInfo const & info, __out BOOLEAN *runningLatestStatus)
    {
        HRESULT hr = E_FAIL;
        *runningLatestStatus = false;

        // Create ETW sessions for traces.
        auto pSessionProperties = GetSessionProperties(info, 0);
        hr = QueryTrace(NULL, info.SessionName.c_str(), pSessionProperties.get());
        if (ERROR_MORE_DATA == hr)
        {
            pSessionProperties = GetSessionProperties(info, DefaultAdditionalBytesForName);
            hr = QueryTrace(NULL, info.SessionName.c_str(), pSessionProperties.get());
        }

        if (ERROR_WMI_INSTANCE_NOT_FOUND == hr)
        {
            return ERROR_SUCCESS;
        }

        if (ERROR_SUCCESS != hr)
        {
            return hr;
        }

        // We can compare desired vs actual session properties
        auto pDesiredProperties = GetSessionProperties(info, 0);

        // These are the properties we explicitly set.
        auto mismatchFound = false;
        if ((pSessionProperties->LogFileMode & 0xF) != (pDesiredProperties->LogFileMode & 0xF) || // higher order bits can be set by default
            pSessionProperties->MaximumFileSize != pDesiredProperties->MaximumFileSize ||
            pSessionProperties->BufferSize != pDesiredProperties->BufferSize ||
            pSessionProperties->FlushTimer != pDesiredProperties->FlushTimer)
        {
            mismatchFound = true;
        }

        if (mismatchFound)
        {
            WriteInfo(
                TraceType,
                "Session properties {0}, {1}, {2}, {3} differs from desired {4}, {5}, {6}, {7}.",
                pSessionProperties->LogFileMode,
                pSessionProperties->MaximumFileSize,
                pSessionProperties->BufferSize,
                pSessionProperties->FlushTimer,
                pDesiredProperties->LogFileMode,
                pDesiredProperties->MaximumFileSize,
                pDesiredProperties->BufferSize,
                pDesiredProperties->FlushTimer);
            return ERROR_SUCCESS;
        }

        auto sessionFilePath = wstring((wchar_t*)((BYTE*)pSessionProperties.get() + pSessionProperties->LogFileNameOffset));
        auto desiredFilePath = wstring((wchar_t*)((BYTE*)pDesiredProperties.get() + pDesiredProperties->LogFileNameOffset));

        if (TraceSessionFilePathCompare(sessionFilePath, desiredFilePath) != 0)
        {
            WriteInfo(
                TraceType,
                "Session LogFileName differs from desired. Actual {0}. Desired {1}.",
                sessionFilePath,
                desiredFilePath);
            return ERROR_SUCCESS;
        }

        *runningLatestStatus = true;
        return ERROR_SUCCESS;
    }

    wstring TraceSession::GetSessionName(wstring const & sessionName)
    {
        if (testKeyword_.empty())
        {
            return sessionName;
        }
        else
        {
            return wformatString("{0}.{1}", sessionName, testKeyword_);
        }
    }

    int TraceSession::TraceSessionFilePathCompare(wstring const & a, wstring const & b)
    {
        // Format: {folderpath}\{session}_{version}_{nonce}_{index}.etl" 
        // We want to compare up and including to the version field.
        auto aNormalized = Common::Path::GetFullPath(a);
        auto bNormalized = Common::Path::GetFullPath(b);

        auto apos = aNormalized.rfind('_');
        apos = aNormalized.rfind('_', apos - 1);

        auto bpos = bNormalized.rfind('_');
        bpos = bNormalized.rfind('_', bpos - 1);

        if (apos != bpos)
        {
            return -1;
        }

        return aNormalized.compare(0, apos, bNormalized, 0, apos);
    }
#else
    
    HRESULT TraceSession::StartLinuxTraceSessions(int linuxTraceDiskBufferSizeInMB, bool keepExistingSFSession)
    {
        wstring fabricCodePathW, fabricLogRootW;
        auto error = FabricEnvironment::GetFabricCodePath(fabricCodePathW);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Could not find FabricCodePath. Unable to start trace sessions: ErrorCode={0}", error);
            return error.ToHResult();
        }

        error = FabricEnvironment::GetFabricLogRoot(fabricLogRootW);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Could not find FabricLogRoot. Unable to start trace sessions: ErrorCode={0}", error);
            return error.ToHResult();
        }

        wstring versionFile = Path::Combine(fabricCodePathW, L"ClusterVersion");
        File fileReader;
        error = fileReader.TryOpen(versionFile, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Could not read FabricVersion. Unable to start trace sessions: ErrorCode={0}", error);
            return error.ToHResult();
        }

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        wstring fabricVersion;
        StringWriter(fabricVersion).Write(text);

        wstring lttngHandlerPath = Path::Combine(fabricCodePathW, L"lttng_handler.sh");
        string startTraceSessionCmd = StringUtility::Utf16ToUtf8(wformatString("{0} {1} {2} {3}", lttngHandlerPath, fabricVersion, linuxTraceDiskBufferSizeInMB, keepExistingSFSession ? 1 : 0));
        
        if (0 != system(startTraceSessionCmd.c_str()))
        {
            WriteError(TraceType, "Unable to start trace sessions: ErrorCode={0}", ErrorCode::FromHResult(E_FAIL));
            return E_FAIL;
        }
        else
        {
            WriteNoise(TraceType, "Trace sessions successfully started");
            return ERROR_SUCCESS;
        }
    }
#endif

    HRESULT TraceSession::TraceResult(
        wstring const & method,
        HRESULT hr)
    {
        if (ERROR_SUCCESS != hr)
        {
            WriteInfo(
                TraceType,
                "Method {0} failed with HRESULT: {1}",
                method, hr);
        }
        else
        {
            WriteNoise(
                TraceType,
                "Method {0} Succeeded",
                method);
        }

        return hr;
    }
}
