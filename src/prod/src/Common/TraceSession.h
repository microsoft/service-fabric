// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#ifndef PLATFORM_UNIX
    static const int ProvidersPerSession = 11;

    namespace TraceSessionKind
    {
        enum Enum
        {
            Invalid     = 0,
            Fabric      = 1,
            Lease       = 2,
            AppInfo     = 3,
            Query       = 4,
            Operational = 5,
            SFBDMiniport = 6
        };
    }
#endif

    class TraceSession :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:

        DENY_COPY(TraceSession);

        typedef std::shared_ptr<TraceSession> TraceSessionSPtr;
        static TraceSessionSPtr Instance();

        Common::ErrorCode StartTraceSessions();
        Common::ErrorCode StopTraceSessions();
#ifndef PLATFORM_UNIX
        Common::ErrorCode StartTraceSession(TraceSessionKind::Enum traceSessionKind);
        Common::ErrorCode StopTraceSession(TraceSessionKind::Enum traceSessionKind);
#endif
    private:

        TraceSession();

        HRESULT TraceResult(
            std::wstring const & method,
            HRESULT exitCode);

#ifndef PLATFORM_UNIX
        struct DataCollectorSetInfo
        {
            TraceSessionKind::Enum TraceSessionKind;
            std::wstring SessionName;
            std::wstring TraceFolderName;
            std::wstring TraceFileName;
            Common::Guid ProviderGuids[ProvidersPerSession];
            UCHAR TraceLevel;
            ULONGLONG Keyword[ProvidersPerSession];
            ULONGLONG KeywordAll[ProvidersPerSession];
            ULONG BufferSizeInKB;
            ULONG FileSizeInMB;
            ULONG FlushIntervalInSeconds;
        };


        unique_ptr<EVENT_TRACE_PROPERTIES, decltype(free)*> GetSessionProperties(DataCollectorSetInfo const & info, uint additionalBytes);

        HRESULT StartTraceSession(DataCollectorSetInfo const & info);
        HRESULT StopTraceSession(DataCollectorSetInfo const & info);

        HRESULT TraceSession::Prepare(DataCollectorSetInfo const & info, BOOLEAN *runningLatestStatus);

        HRESULT TraceSession::GetTraceSessionStatus(DataCollectorSetInfo const & info, BOOLEAN *runningLatestStatus);

        std::wstring GetSessionName(std::wstring const & sessionName);

        wstring TraceSession::GetTraceFolder(DataCollectorSetInfo const & info);

        int TraceSession::TraceSessionFilePathCompare(std::wstring const & a, std::wstring const & b);
#else
        HRESULT StartLinuxTraceSessions(int linuxTraceDiskBufferSizeInMB, bool keepExistingSFSession);
#endif
    private:
#ifndef PLATFORM_UNIX
        static const ULONG DefaultTraceBufferSizeInKB = 128;
        static const ULONG DefaultTraceFileSizeInMB = 64;
        static const ULONG DefaultTraceFlushIntervalInSeconds = 60;
        static const ULONG DefaultAppInfoTraceBufferSizeInKB = 8;
        static const ULONG DefaultAppInfoTraceFlushIntervalInSeconds = 60;
        static const ULONG DefaultCircularFileSizeMultiplier = 4;
        static const ULONG DefaultAdditionalBytesForName = 1024;

        // Trace Provider GUIDS
        static Common::Global<Common::Guid> LeaseDriverGuid;
        static Common::Global<Common::Guid> SFBDMiniportDriverGuid;
        static Common::Global<Common::Guid> KTLGuid;
        static Common::Global<Common::Guid> FabricGuid;
        static Common::Global<Common::Guid> MSFIGuid;
        static Common::Global<Common::Guid> ProgrammingModel_ActorGuid;
        static Common::Global<Common::Guid> SF_ClientLib_HttpGuid;
        static Common::Global<Common::Guid> SFPOSCoordinatorServiceGuid;
        static Common::Global<Common::Guid> SFPOSNodeAgentServiceGuid;
        static Common::Global<Common::Guid> SFPOSNodeAgentNTServiceGuid;
        static Common::Global<Common::Guid> SFPOSNodeAgentSFUtilityGuid;
        static Common::Global<Common::Guid> ProgrammingModel_ServicesGuid;
        static Common::Global<Common::Guid> ReliableCollectionGuid;
        static Common::Global<Common::Guid> AzureFilesVolumeDriverGuid;

        static Common::GlobalWString FabricTraceFileName;
        static Common::GlobalWString SFBDMiniportTraceFileName;
        static Common::GlobalWString LeaseTraceFileName;
        static Common::GlobalWString AppInfoTraceFileName;
        static Common::GlobalWString AppInfoTraceFolderName;
        static Common::GlobalWString QueryTraceFileName;
        static Common::GlobalWString QueryTraceFolderName;
        static Common::GlobalWString OperationalTraceFileName;
        static Common::GlobalWString OperationalTraceFolderName;

        static Common::GlobalWString FabricTraceSessionName;
        static Common::GlobalWString LeaseTraceSessionName;
        static Common::GlobalWString SFBDMiniportTraceSessionName;
        static Common::GlobalWString AppInfoTraceSessionName;
        static Common::GlobalWString QueryTraceSessionName;
        static Common::GlobalWString FabricCountersSessionName;
        static Common::GlobalWString OperationalTraceSessionName;

        static Common::GlobalWString FabricDataRoot;

        static Common::GlobalWString CurrentExeVersion;

        std::vector<DataCollectorSetInfo> dataCollectorSets;
        std::wstring testKeyword_;
        std::wstring tracePath_;
        BOOLEAN enableCircularTraceSession_;
#endif
    };
}
