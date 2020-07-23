// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseDefragmenter
        : public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::EseStore>
    {
    private:

        EseDefragmenter(__in EseLocalStore &);

    public:

        static std::shared_ptr<EseDefragmenter> CreateAndStart(__in EseLocalStore &);

        virtual ~EseDefragmenter();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        void Cancel();

        Common::ErrorCode StartDefragmentation();
        Common::ErrorCode StopDefragmentation();

    private:

        void Start();
        void DoStart();
        void ScheduleStartRetry();
        void ScheduleDefragmentation(Common::TimeSpan const);
        void ScheduleDefragmentationRetry();
        void DefragmentationCallback();
        void ScheduleTimerCallback(
            Common::Timer::TimerCallback const & timerCallback, 
            Common::TimeSpan const targetDelay,
            std::wstring const & debugTag);

        void IncrementLocalStoreRefCount();
        void DecrementLocalStoreRefCount();

        Common::ErrorCode InitializeSession_ReadLock();

        // The current version (15.0.805.6) of ESE used has a bug where trying to 
        // degragment JET database with JET_DBID > 255 hits AV. As a temporary workaround
        // skip doing defragmentation when JET_DBID > 255.
        bool IsJetDbidInSupportedRange();

        static JET_ERR DefragmentationCompletedCallback(
            JET_SESID,
            JET_DBID,
            JET_TABLEID,
            JET_CBTYP,
            void *,
            void *,
            void *,
            JET_API_PTR);

    private:

        class LocalStoreRoot;

        // access to both eseStore_ and settings_ must be protected by
        // isCanceled_ flag, which gates EseLocalStore destruction
        //
        EseLocalStore & eseStore_;
        EseLocalStoreSettings const & settings_;
        mutable std::wstring traceId_;
        
        EseSessionSPtr session_;
        EseDatabaseSPtr database_;

        RWLOCK(EseDefragmenterLock, lock_);
        bool isCanceled_;
        Common::TimerSPtr defragTimer_;
    };
}
