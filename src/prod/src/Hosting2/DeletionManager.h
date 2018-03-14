// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // downloads application and service package from imagestore
    class DeletionManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(DeletionManager)

    public:
        DeletionManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~DeletionManager();
        
    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        void ScheduleCleanup(Common::TimeSpan const & delay);

        void StartCleanup();
        void OnCleanupCompleted(Common::AsyncOperationSPtr const &);

        Common::ErrorCode RegisterCleanupAsyncOperation(Common::AsyncOperationSPtr const &);
        void UnregisterCleanupAsyncOperation();
                
    private:                
        class CleanupAppInstanceFoldersAsyncOperation;
        class CleanupAppTypeFoldersAsyncOperation;
        class CleanupAsyncOperation;

        typedef std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> ContentSet;
        typedef std::map<std::wstring, ContentSet, Common::IsLessCaseInsensitiveComparer<std::wstring>> AppTypeContentMap;
        typedef std::map<std::wstring, ContentSet, Common::IsLessCaseInsensitiveComparer<std::wstring>> containerImageContentMap;
        typedef std::map<std::wstring, std::set<ServiceModel::ApplicationIdentifier>, Common::IsLessCaseInsensitiveComparer<std::wstring>> AppTypeInstanceMap;

    private:
        HostingSubsystem & hosting_;      
        std::vector<std::wstring> containerImagesToSkip_;
        Common::ExclusiveLock lock_;
        Common::TimerSPtr cacheCleanupScanTimer_;
        Common::AsyncOperationSPtr cleanupAsyncOperation_;
    };
}
