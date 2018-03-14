// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class NativeImageStoreProgressState 
            : public Api::IFileStoreServiceClientProgressEventHandler   
            , public Common::ComponentRoot
            , public Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::NativeImageStoreClient>::WriteError;
            
        public:

            NativeImageStoreProgressState(
                Api::INativeImageStoreProgressEventHandlerPtr const &, 
                std::wstring const & operationName,
                ServiceModel::ProgressUnitType::Enum const itemType);

            NativeImageStoreProgressState(
                Api::INativeImageStoreProgressEventHandlerPtr &&, 
                std::wstring const & operationName,
                ServiceModel::ProgressUnitType::Enum const itemType);

            //
            // IFileStoreServiceClientProgressEventHandler
            //
            void IncrementReplicatedFiles(size_t) override;
            void IncrementTotalFiles(size_t value) override;

            void IncrementTransferCompletedItems(size_t) override;
            void IncrementTotalTransferItems(size_t value) override;

            void IncrementCompletedItems(size_t value) override;
            void IncrementTotalItems(size_t value) override;

            void StopDispatchingUpdates();

        private:

            typedef enum
            {
                ApplicationPackageTransferProgress,
                ApplicationPackageReplicateProgress
            } ProgressType;

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return traceId_; }

            void OnCtor(std::wstring const & operationName);
            bool TryGetEventHandler(__out Api::INativeImageStoreProgressEventHandlerPtr &);
            void StartTimerIfNeeded();
            void ProgressTimerCallback();

            void StartUploadTimerIfNeeded();
            void ProgressUploadTimerCallback();

            ServiceModel::ProgressUnitSPtr GetProgressUnit(ProgressType const type);
            ServiceModel::ProgressUnitSPtr SetProgressUnit(ProgressType const type);

            std::wstring traceId_;
            Api::INativeImageStoreProgressEventHandlerPtr eventHandler_;
            ServiceModel::ProgressUnitType::Enum itemType_;
            Common::atomic_uint64 completedItems_;
            Common::atomic_uint64 totalItems_;

            Common::RwLock lock_;
            Common::TimerSPtr updateTimer_;
            Common::atomic_bool isTimerActive_;

            Common::TimerSPtr updateUploadTimer_;
            Common::atomic_bool isUploadTimerActive_;
            Common::DateTime lastUpdateUploadTime_;

            std::map<ProgressType, ServiceModel::ProgressUnitSPtr> units_;
        };

        typedef std::shared_ptr<NativeImageStoreProgressState> NativeImageStoreProgressStateSPtr;
    }
}
