// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
#define BIND_TO_THREAD( session ) \
    BIND_TO_THREAD_TAG( session, TO_WSTRING( __FUNCTION__ ) ) \

#define BIND_TO_THREAD_TAG( session, tag ) \
    JET_ERR jetErr = JET_errSuccess; \
    BIND_TO_THREAD_IMPL( session, jetErr, tag) \

#define BIND_TO_THREAD_SET_ERR( session, jetErr ) \
    BIND_TO_THREAD_IMPL( session, jetErr, TO_WSTRING( __FUNCTION__ ) ) \
    
#define BIND_TO_THREAD_IMPL( session, jetErr, tag ) \
    Store::EseSession::BindToThread bind( session, tag ); \
    jetErr = bind.Initialize(); \

    class EseSession : public std::enable_shared_from_this<EseSession>
    {
        DENY_COPY(EseSession);
    private:
        EseSession(EseInstanceSPtr const & instance);

    public:

        static std::shared_ptr<EseSession> CreateSPtr(EseInstanceSPtr const & instance);
        ~EseSession();

        __declspec (property(get=get_Instance)) EseInstanceSPtr const & Instance;
        __declspec (property(get=get_SessionId)) JET_SESID SessionId;
        __declspec (property(get=get_HasUnrecoverableError)) bool HasUnrecoverableError;

        EseInstanceSPtr const & get_Instance() const { return instance_; }
        JET_SESID get_SessionId() const { return sessionId_; }
        bool get_HasUnrecoverableError() const { return hasUnrecoverableError_; }

        JET_ERR Initialize();
        JET_ERR BeginTransaction();
        JET_ERR CommitTransaction();
        JET_ERR RollbackTransaction();

        bool TryWaitForInvalidSessionThread() const;

        Common::AsyncOperationSPtr BeginCommitTransaction(
            EseStorePerformanceCounters const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);

        Common::ErrorCode EndCommitTransaction(Common::AsyncOperationSPtr const & operation);

        struct BindToThread
        {
            DENY_COPY(BindToThread);
        public:
            BindToThread(EseSession const & session, std::wstring const & tag);
            ~BindToThread();

            JET_ERR Initialize();
            
        private:
            EseSession const & session_;
            std::wstring tag_;
            EseInstance::InstanceRef instanceRef_;
            bool sessionContextSet_;
        };

    private:
        void Abort();

        EseInstanceSPtr const instance_;
        JET_SESID sessionId_;
        mutable bool hasUnrecoverableError_;
        mutable volatile DWORD currentThread_;

        const static DWORD InvalidThreadId = 0xffffffff;

        friend class EseInstance;
        friend class EseLocalStore;
    };

    typedef std::shared_ptr<EseSession> EseSessionSPtr;
}
