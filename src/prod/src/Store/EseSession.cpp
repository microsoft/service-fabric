// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    EseSession::EseSession(EseInstanceSPtr const & instance)
        : instance_(instance)
        , sessionId_(JET_sesidNil)
        , hasUnrecoverableError_(false)
        , currentThread_(InvalidThreadId)
    {
    }

    EseSession::~EseSession()
    {
#if DBG
        instance_->AssertSessionIsNotActive(*this);
#endif
        if (sessionId_ != JET_sesidNil)
        {
            EseInstance::InstanceRef ref(*instance_);

            if (ref.IsValid)
            {
                CALL_ESE_NOTHROW(JetEndSession(sessionId_, NULL));
            }
        }
    }

    EseSessionSPtr EseSession::CreateSPtr(EseInstanceSPtr const & instance)
    {
        return shared_ptr<EseSession>(new EseSession(instance));
    }

    JET_ERR EseSession::Initialize()
    {
        ASSERT_IF(sessionId_ != JET_sesidNil, "Cannot initialize twice");

        JET_ERR jetErr = JET_errTermInProgress;

        EseInstance::InstanceRef ref(*instance_);

        if (ref.IsValid)
        {
            jetErr = CALL_ESE_NOTHROW(JetBeginSession(ref.InstanceId, &sessionId_, NULL, NULL));
        }

        return jetErr;
    }

    JET_ERR EseSession::BeginTransaction()
    {
        BIND_TO_THREAD( *this )

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(JetBeginTransaction(SessionId));
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = instance_->AddActiveSession(shared_from_this());

            // AddActiveSession will internally call EseSession::Abort on error
        }

        return jetErr;
    }

    JET_ERR EseSession::CommitTransaction()
    {
        BIND_TO_THREAD( *this )

        JET_ERR jetErrSyncCommit = JET_errSuccess;

        if (JET_errSuccess == jetErr)
        {
            instance_->RemoveActiveSession(*this);

            // First call commit with the lazy bit. This does not make it durable.
            jetErr = CALL_ESE_NOTHROW(JetCommitTransaction(SessionId, JET_bitCommitLazyFlush));

            // Now the session is not in the scope of a transaction, call another commit with JET_bitWaitAllLevel0Commit to
            // make it durable. This sequence guarantees any previous lazy transactions (which includes async ones) are made durable
            jetErrSyncCommit = CALL_ESE_NOTHROW(JetCommitTransaction(SessionId, JET_bitWaitAllLevel0Commit));

            jetErr = (JET_errSuccess == jetErr) ? jetErrSyncCommit : jetErr;
        }
        
        auto err = instance_->JetToErrorCode(jetErrSyncCommit);
        if (err.IsError(ErrorCodeValue::StoreFatalError))
        {
            // If sync commit returns one of the above error codes, it indicates a fatal error in ESE and the pending commits may never complete.
            // We must cancel any pending commits that are in the instance
            instance_->OnFatalCommitError(err);
        }

        // Return the first error encountered
        return jetErr;
    }

    Common::AsyncOperationSPtr EseSession::BeginCommitTransaction(
        EseStorePerformanceCounters const & perfCounters,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & state)
    {
        AsyncOperationSPtr asyncOperation;
        bool addedPendingCommit = false;

        {
            EseInstance::InstanceRef ref(*instance_);

            if (ref.IsValid)
            {
                auto casted = make_shared<EseCommitAsyncOperation>(
                    shared_from_this(),
                    instance_->Settings, 
                    perfCounters, 
                    timeout, 
                    callback, 
                    state);

                asyncOperation = casted;

                casted->Start(casted);

                if (!asyncOperation->IsCompleted)
                {
                    // Do not use BindToThread around here since the async operation can get completed
                    // if ESE completed the durable commit sychronously
                    addedPendingCommit = instance_->TryAddPendingCommit(casted);
                }
            }
        }

        if (!asyncOperation)
        {
            // reference was not valid
            return AsyncOperation::CreateAndStart<EseCommitAsyncOperation>(
                instance_->JetToErrorCode(JET_errTermInProgress), 
                perfCounters,
                callback, 
                state);
        }
        else if (!addedPendingCommit)
        {
            asyncOperation->TryComplete(asyncOperation);
        }

        return asyncOperation;
    }

    Common::ErrorCode EseSession::EndCommitTransaction(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<EseCommitAsyncOperation>(operation)->Error;
    }

    JET_ERR EseSession::RollbackTransaction()
    {
        BIND_TO_THREAD( *this )

        if (JET_errSuccess == jetErr || JET_errSessionSharingViolation == jetErr)
        {
            // There may be outstanding enumerations if this transaction is being
            // externally forced to abort - JetSetSessionContext may fail with
            // JET_errSessionSharingViolation in this case. Remove the active
            // session anyway.
            //
            instance_->RemoveActiveSession(*this);

            jetErr = CALL_ESE_NOTHROW(JetRollback(SessionId, 0));
        }

        return jetErr;
    }

    void EseSession::Abort()
    {
        JET_ERR jetErr = JET_errSuccess;

        // Do not use BindToThread here since EseInstance is aborting
        // Abort can be called in 2 cases:
        //      - A transaction that just got started (i.e. call from EseInstance::AddActiveSession)
        //        In this case the session is already bound to the current thread.
        //      - A transaction that has been active and in the EseInstance::activeSessions_ list
        //        Here the session will not be bound to any thread, and hence need to be bound
        //
        if (InvalidThreadId == currentThread_)
        {
            jetErr = JetSetSessionContext(sessionId_, reinterpret_cast<JET_API_PTR>(this));
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = JetRollback(SessionId, 0);

            if (JET_errSuccess != jetErr)
            {
                EseLocalStore::WriteWarning("check", "EseSession::Abort : 'JetRollback' returned '{0}'", jetErr);
            }
        }
        else
        {
            EseLocalStore::WriteWarning("check", "EseSession::Abort : 'JetSetSessionContext' returned '{0}'", jetErr);
        }
    }

    bool EseSession::TryWaitForInvalidSessionThread() const
    {
        DWORD threadId = InvalidThreadId;

        for (auto retry=0; retry<StoreConfig::GetConfig().InvalidSessionThreadRetryCount; ++retry)
        {
            threadId = currentThread_;
            if (threadId == EseSession::InvalidThreadId)
            {
                return true;
            }
            else
            {
                EseLocalStore::WriteInfo(
                    Constants::StoreSource,
                    "Current thread id {0} is not invalid thread id. Retrying...", 
                    threadId);

                Sleep(StoreConfig::GetConfig().InvalidSessionThreadRetryDelayMilliseconds);
            }
        }

        return false;
    }

    EseSession::BindToThread::BindToThread(
        EseSession const & session,
        wstring const & tag)
        : session_(session)
        , tag_(tag)
        , instanceRef_(*session.instance_) // This takes a soft lock on EseInstance
        , sessionContextSet_(false)
    {
    }

    JET_ERR EseSession::BindToThread::Initialize()
    {
        JET_ERR jetErr = JET_errTermInProgress;

        if (instanceRef_.IsValid)
        {
            auto tid = GetCurrentThreadId();
            if (session_.currentThread_ != tid)
            {
                // There may still be outstanding enumerations after the transaction destructs.
                // Wait a short period of time for these enumerations to observe the destructed
                // parent transaction, which fails the enumeration and prevents further read
                // attempts. Functionally, it's fine to return the error immediately, but waiting
                // a small period results in cleaner logs.
                //
                if (!session_.TryWaitForInvalidSessionThread())
                {
                    EseLocalStore::WriteWarning(
                        Constants::StoreSource,
                        "Multithreaded transactions not supported: Current thread id {0} != session thread id {1}. session={2} tag={3}", 
                        tid,
                        session_.currentThread_,
                        session_.SessionId,
                        tag_);

                    return JET_errSessionContextAlreadySet;
                }

                jetErr = CALL_ESE_NOTHROW_TAG(JetSetSessionContext(session_.SessionId, reinterpret_cast<JET_API_PTR>(&session_)), tag_);

                if (JET_errSuccess == jetErr)
                {
                    session_.currentThread_ = tid;

                    // sessionContextSet_ is used to handle nested calls to BindToThread
                    sessionContextSet_ = true;
                }
            }
            else
            {
                jetErr = JET_errSuccess;
            }
        }

        return jetErr;
    }

    EseSession::BindToThread::~BindToThread()
    {
        if (sessionContextSet_)
        {
            auto tid = GetCurrentThreadId();
            ASSERT_IF(
                session_.currentThread_ != tid,
                "Current thread id {0} != session thread id {1}.", tid, static_cast<DWORD>(session_.currentThread_));

            for (auto retry=0; retry<StoreConfig::GetConfig().InvalidSessionThreadRetryCount; ++retry)
            {
                JET_ERR jetErr = CALL_ESE_NOTHROW_TAG(JetResetSessionContext(session_.SessionId), tag_);
                if (JET_errSuccess == jetErr)
                {
                    session_.currentThread_ = InvalidThreadId;

                    return;
                }
                else
                {
                    EseLocalStore::WriteInfo(
                        Constants::StoreSource,
                        "Failed to reset session context. Retrying... tag={0}",
                        tag_); 

                    Sleep(StoreConfig::GetConfig().InvalidSessionThreadRetryDelayMilliseconds);
                }
            }

            // Reset currentThread_ so that we don't spend time in
            // TryWaitForInvalidSessionThread() anymore. Repeated 
            // attempts to JetSetSessionContext() will now fail 
            // immediately with JET_errSessionContextAlreadySet.
            //
            session_.currentThread_ = InvalidThreadId;
            session_.hasUnrecoverableError_ = true;

            // For now, attempt to catch offending threads by asserting early
            //
            TRACE_LEVEL_AND_ASSERT( EseLocalStore::WriteWarning, Constants::StoreSource,  "Failed to reset session context" );
        }
    }
}
