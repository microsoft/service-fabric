// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EtwSessionProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(EtwSessionProvider)

    public:
        explicit EtwSessionProvider(
            Common::ComponentRoot const & root, 
            std::wstring const & nodeId,
            std::wstring const & deploymentFolder);
        virtual ~EtwSessionProvider();

        Common::ErrorCode SetupServiceEtw(
            ServiceModel::ServicePackageDescription const & servicePackageDescription,
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext);

        void CleanupServiceEtw(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext);

    protected:
        // ****************************************************
        // FabricComponent specific methods
        // ****************************************************
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        Common::ErrorCode ComputePropertiesBufferSize(
            std::wstring const & logFileName,
            __out ULONG & propertiesBufferSize);

        PEVENT_TRACE_PROPERTIES GetEventTraceProperties(Common::ByteBuffer const & buffer);

        Common::ErrorCode StartEtwTraceSession();
        Common::ErrorCode StopEtwTraceSession();

        Common::ErrorCode EnableGuidForTraceSession(std::wstring const & providerGuid);
        Common::ErrorCode DisableGuidForTraceSession(std::wstring const & providerGuid, bool removeGuidFromMap);
        void DisableAllGuidsForTraceSession();
        Common::ErrorCode DisableGuidForTraceSessionLocked(std::wstring const & providerGuid, bool removeGuidFromMap);
        Common::ErrorCode DisableGuidForTraceSessionLocked(std::map<std::wstring, int>::iterator & etwGuidMapEntry, bool removeGuidFromMap);

    private:
        std::wstring traceSessionName_;
        std::wstring tracePath_;
        TRACEHANDLE traceSessionHandle_;
        std::map<std::wstring, int> etwGuidMap_;
        Common::RwLock etwGuidMapLock_;
        bool etwGuidMapIsClosed_;
        static const ULONG DefaultTraceSessionFlushIntervalInSeconds = 60;
        static const std::wstring TraceSessionNamePrefix;
        static const std::wstring TracesFolderName;
        static const std::wstring LogFileNamePrefix;
        static const std::wstring LogFileNameSuffix;
    };
}
