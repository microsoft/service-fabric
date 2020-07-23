// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("VersionedServicePackage");

class VersionedServicePackage::ServiceTypeRegistrationTimeoutTracker
{
    DENY_COPY(ServiceTypeRegistrationTimeoutTracker)

public:
    ServiceTypeRegistrationTimeoutTracker(VersionedServicePackage & owner)
        : owner_(owner)
        , lock_()
        , map_()
    {
    }

    void Reset(vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
    {
        AcquireWriteLock writeLock(lock_);

        map_.clear();

        for (auto const & serviceType : serviceTypeInstanceIds)
        {
            map_[serviceType.ServiceTypeName] = DateTime::Now();
        }
    }

    bool HasRegistrationTimedout(wstring const & serviceTypeName)
    {
        AcquireReadLock readLock(lock_);

        auto iter = map_.find(serviceTypeName);
        if (iter != map_.end())
        {
            auto elapsedTime = DateTime::Now() - iter->second;

            if (elapsedTime > HostingConfig::GetConfig().ServiceTypeRegistrationTimeout &&
                owner_.GetState() == VersionedServicePackage::Opened)
            {
                return true;
            }
        }

        return false;
    }

    void OnServiceTypesUnregistered(
        vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
    {
        wstring serviceTypeNames;
        wstring unexpectedServiceTypeNames;

        {
            AcquireWriteLock writeLock(lock_);

            for (auto const & serviceType : serviceTypeInstanceIds)
            {
                serviceTypeNames = 
                    serviceTypeNames.empty() ? 
                    serviceType.ServiceTypeName :
                    wformatString("{0}, {1}", serviceTypeNames, serviceType.ServiceTypeName);

                auto iter = map_.find(serviceType.ServiceTypeName);
                if (iter != map_.end())
                {
                    iter->second = DateTime::Now();
                }
                else
                {
                    unexpectedServiceTypeNames =
                        unexpectedServiceTypeNames.empty() ?
                        serviceType.ServiceTypeName :
                        wformatString("{0}, {1}", unexpectedServiceTypeNames, serviceType.ServiceTypeName);
                }
            }
        }

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "OnServiceTypesUnregistered: ServicePackageInstanceId={0}, ServiceTypeNames=[{1}], UnexpectedServiceTypeNames=[{2}]",
            owner_.ServicePackageObj.Id,
            serviceTypeNames,
            unexpectedServiceTypeNames);
    }

    void ResetTrackingTime()
    {
        AcquireWriteLock writeLock(lock_);

        for (auto & kvPair : map_)
        {
            kvPair.second = DateTime::Now();
        }
    }

private:
    VersionedServicePackage & owner_;
    mutable RwLock lock_;
    map<wstring, DateTime, IsLessCaseInsensitiveComparer<wstring>> map_;
};

class VersionedServicePackage::ActivatorCodePackageOperationTracker
{
    DENY_COPY(ActivatorCodePackageOperationTracker)

private:
    class DrainWaitAsyncOperation
        : public AsyncOperation
        , protected TextTraceComponent<TraceTaskCodes::Hosting>
    {
        DENY_COPY(DrainWaitAsyncOperation)

    public:
        DrainWaitAsyncOperation(
            _In_ ActivatorCodePackageOperationTracker & owner,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
            , owner_(owner)
        {
        }

        virtual ~DrainWaitAsyncOperation()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<DrainWaitAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            {
                AcquireWriteLock writeLock(owner_.lock_);

                ASSERT_IF(
                    owner_.drainWaiter_ != nullptr, 
                    "There can only be one drain waiter for ApplicationHostCodePackageOperationTracker");

                if (owner_.map_.size() == 0)
                {
                    this->TryComplete(thisSPtr, ErrorCode::Success());
                    return;
                }

                //
                // TODO: To optimize explicitly cancel the pending operations.
                //

                owner_.drainWaiter_ = thisSPtr;
            }

            WriteNoise(
                TraceType,
                owner_.owner_.TraceId,
                "ApplicationHostCodePackageOperationTracker: Waiting for {} pending operations to drain.",
                owner_.map_.size());
        }

    private:
        ActivatorCodePackageOperationTracker & owner_;
    };

public:
    ActivatorCodePackageOperationTracker(VersionedServicePackage & owner)
        : owner_(owner)
        , lock_()
        , map_()
        , drainWaiter_()
    {
    }

    bool TryTrack(AsyncOperationSPtr const & operation, _Out_ uint64 & operationId)
    {
        //
        // If there is a drain operation waiting for pending operations
        // to drain, it means activator CP has terminated and any new 
        // operation should not be allowed.
        //

        {
            AcquireReadLock readLock(lock_);

            if (drainWaiter_ != nullptr)
            {
                return false;
            }
        }

        {
            AcquireWriteLock writeLock(lock_);

            if (drainWaiter_ != nullptr)
            {
                return false;
            }

            operationId = owner_.Hosting.GetNextSequenceNumber();
            map_.insert(make_pair(operationId, operation));
        }

        return true;
    }

    void Untrack(uint64 operationId)
    {
        AcquireWriteLock writeLock(lock_);

        map_.erase(operationId);

        if (map_.size() == 0 && drainWaiter_ != nullptr)
        {
            drainWaiter_->TryComplete(drainWaiter_, ErrorCode::Success());
            drainWaiter_ = nullptr;
        }
    }

    AsyncOperationSPtr BeginCancelPendingOperationsAndWaitForDrain(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DrainWaitAsyncOperation>(
            *this,
            callback,
            parent);
    }

    void EndCancelPendingOperationsAndWaitForDrain(
        AsyncOperationSPtr const & operation)
    {
        DrainWaitAsyncOperation::End(operation).ReadValue();
    }

private:
    VersionedServicePackage & owner_;
    mutable RwLock lock_;
    map<uint64, AsyncOperationSPtr> map_;
    AsyncOperationSPtr drainWaiter_;
};

class VersionedServicePackage::ApplicationHostActivateCodePackageAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ApplicationHostActivateCodePackageAsyncOperation)

public:
    ApplicationHostActivateCodePackageAsyncOperation(
        _In_ VersionedServicePackage & owner,
        wstring const & codePackageName,
        std::vector<wchar_t> const & envBlock,
        ApplicationHostContext const & appHostContext,
        CodePackageContext const & codePackageContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageName_(codePackageName)
        , envBlock_(envBlock)
        , appHostContext_(appHostContext)
        , codePackageContext_(codePackageContext)
        , timeout_(timeout)
        , codePackage_()
        , operationId_(0)
    {
    }

    virtual ~ApplicationHostActivateCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostActivateCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto success = owner_.operationTracker_->TryTrack(thisSPtr, operationId_);
        if (!success)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostAbortCodePackage: Tracking failed. CPName={0}, CPContext={1}, AppHostContext={2}",
                codePackageName_,
                codePackageContext_,
                appHostContext_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto error = this->TryCreateCodePackage();
        if (error.IsError(ErrorCodeValue::AlreadyExists))
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostAbortCodePackage: AlreadyExist. CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_,
                operationId_);

            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }
        else if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ApplicationHostActivateCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
            codePackageName_,
            codePackageContext_,
            appHostContext_,
            operationId_);

        auto operation = codePackage_->BeginActivate(
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateCodePackage(operation, false); 
            },
            thisSPtr);

        this->FinishActivateCodePackage(operation, true);
    }

    void OnCompleted()
    {
        owner_.operationTracker_->Untrack(operationId_);
    }

private:
    void FinishActivateCodePackage(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = codePackage_->EndActivate(operation);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(ApplicationHostActivateCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}, Error={4}.",
            codePackageName_,
            codePackageContext_,
            appHostContext_,
            operationId_,
            error);

        if (!error.IsSuccess())
        {
            codePackage_->AbortAndWaitForTerminationAsync(
                [this, error](AsyncOperationSPtr const & abortOperation)
                {
                    this->RemoveCodePackage();
                    this->TryComplete(abortOperation->Parent, error);
                },
                operation->Parent);
            return;
        }

        this->TryComplete(operation->Parent, error);
    }

    ErrorCode TryCreateCodePackage()
    {
        AcquireWriteLock(owner_.Lock);

        auto iter = owner_.activeCodePackages_.find(codePackageName_);
        if (iter == owner_.activeCodePackages_.end())
        {
            auto error = this->CreateCodePackage();
            if (!error.IsSuccess())
            {
                return error;
            }

            owner_.activeCodePackages_.insert(make_pair(codePackageName_, codePackage_));

            return error;
        }

        return ErrorCode(ErrorCodeValue::AlreadyExists);
    }

    void RemoveCodePackage()
    {
        AcquireWriteLock(owner_.Lock);
        owner_.activeCodePackages_.erase(codePackageName_);
    }

    ErrorCode CreateCodePackage()
    {
        int64 activatorInstanceId;
        auto error = owner_.GetActivatorCodePackageInstanceId(codePackageName_, activatorInstanceId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "ApplicationHostActivateCodePackageAsyncOperation: GetActivatorCodePackageInstanceId failed with Error={0}. CPName={1}, CPContext={2}, AppHostContext={3}, OperationId={4}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_,
                operationId_);

            return error;
        }

        for (auto & dcpDesc : owner_.packageDescription__.DigestedCodePackages)
        {
            if (dcpDesc.Name == codePackageName_)
            {
                wstring userId;
                owner_.GetUserId(dcpDesc.CodePackage.Name, dcpDesc.RunAsPolicy, false, userId);

                wstring setuppointRunas;
                owner_.GetUserId(dcpDesc.CodePackage.Name, dcpDesc.SetupRunAsPolicy, true, setuppointRunas);

                ProcessDebugParameters debugParameters;
                owner_.GetProcessDebugParams(dcpDesc, debugParameters);

                EnvironmentMap envMap;
                LPVOID penvBlock = envBlock_.data();
                Environment::FromEnvironmentBlock(penvBlock, envMap);

                codePackage_ = make_shared<CodePackage>(
                    owner_.ServicePackageObj.HostingHolder,
                    VersionedServicePackageHolder(owner_, owner_.CreateComponentRoot()),
                    dcpDesc,
                    owner_.ServicePackageObj.Id,
                    owner_.ServicePackageObj.InstanceId,
                    owner_.currentVersionInstance__,
                    owner_.ServicePackageObj.ApplicationName,
                    dcpDesc.RolloutVersionValue,
                    owner_.environmentContext__,
                    activatorInstanceId,
                    move(envMap),
                    userId,
                    setuppointRunas,
                    dcpDesc.IsShared,
                    debugParameters,
                    owner_.GetCodePackagePortMappings(dcpDesc.ContainerPolicies, owner_.environmentContext__),
                    owner_.packageDescription__.SFRuntimeAccessDescription.RemoveServiceFabricRuntimeAccess);

                break;
            }
        }

        if (codePackage_ == nullptr)
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "ApplicationHostActivateCodePackage: Requested CP={0} not present in ServicePackage. CPContext={1}, AppHostContext={2}, OperationId={3}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_,
                operationId_);

            return ErrorCode(ErrorCodeValue::CodePackageNotFound);
        }

        return ErrorCode::Success();
    }

private:
    VersionedServicePackage & owner_;
    wstring codePackageName_;
    std::vector<wchar_t> envBlock_;
    ApplicationHostContext  appHostContext_;
    CodePackageContext codePackageContext_;
    TimeSpan timeout_;
    CodePackageSPtr codePackage_;
    uint64 operationId_;
};

class VersionedServicePackage::ApplicationHostDeactivateCodePackageAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ApplicationHostDeactivateCodePackageAsyncOperation)

public:
    ApplicationHostDeactivateCodePackageAsyncOperation(
        _In_ VersionedServicePackage & owner,
        wstring const & codePackageName,
        ApplicationHostContext const & appHostContext,
        CodePackageContext const & codePackageContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageName_(codePackageName)
        , appHostContext_(appHostContext)
        , codePackageContext_(codePackageContext)
        , timeout_(timeout)
        , operationId_(0)
    {
    }

    virtual ~ApplicationHostDeactivateCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostDeactivateCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto success = owner_.operationTracker_->TryTrack(thisSPtr, operationId_);
        if (!success)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostDeactivateCodePackage: Tracking failed. CPName={0}, CPContext={1}, AppHostContext={2}",
                codePackageName_,
                codePackageContext_,
                appHostContext_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto error = this->TryGetCodePackage();
        if (error.IsError(ErrorCodeValue::CodePackageNotFound))
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostDeactivateCodePackage: CodePackageNotFound. CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_,
                operationId_);

            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }
        else if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ApplicationHostDeactivateCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
            codePackageName_,
            codePackageContext_,
            appHostContext_,
            operationId_);

        auto operation = codePackage_->BeginDeactivate(
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeactivateCodePackage(operation, false);
            },
            thisSPtr);

        this->FinishDeactivateCodePackage(operation, true);
    }

    void OnCompleted()
    {
        owner_.operationTracker_->Untrack(operationId_);
    }

private:
    void FinishDeactivateCodePackage(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        codePackage_->EndDeactivate(operation);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(ApplicationHostDeactivateCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
            codePackageName_,
            codePackageContext_,
            appHostContext_,
            operationId_);

        this->RemoveCodePackage();
        this->TryComplete(operation->Parent, ErrorCode::Success());
    }

    ErrorCode TryGetCodePackage()
    {
        AcquireReadLock(owner_.Lock);

        auto iter = owner_.activeCodePackages_.find(codePackageName_);
        if (iter != owner_.activeCodePackages_.end())
        {
            codePackage_ = iter->second;
            return ErrorCode::Success();
        }

        return ErrorCode(ErrorCodeValue::CodePackageNotFound);
    }

    void RemoveCodePackage()
    {
        AcquireWriteLock(owner_.Lock);
        owner_.activeCodePackages_.erase(codePackageName_);
    }

private:
    VersionedServicePackage & owner_;
    wstring codePackageName_;
    ApplicationHostContext  appHostContext_;
    CodePackageContext codePackageContext_;
    TimeSpan timeout_;
    CodePackageSPtr codePackage_;
    uint64 operationId_;
};

class VersionedServicePackage::ApplicationHostAbortCodePackageAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ApplicationHostAbortCodePackageAsyncOperation)

public:
    ApplicationHostAbortCodePackageAsyncOperation(
        _In_ VersionedServicePackage & owner,
        wstring const & codePackageName,
        ApplicationHostContext const & appHostContext,
        CodePackageContext const & codePackageContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageName_(codePackageName)
        , appHostContext_(appHostContext)
        , codePackageContext_(codePackageContext)
        , timeout_(timeout)
        , operationId_(0)
    {
    }

    virtual ~ApplicationHostAbortCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostAbortCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto success = owner_.operationTracker_->TryTrack(thisSPtr, operationId_);
        if (!success)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostAbortCodePackage: Tracking failed. CPName={0}, CPContext={1}, AppHostContext={2}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto error = this->TryGetCodePackage();
        if (error.IsError(ErrorCodeValue::CodePackageNotFound))
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostAbortCodePackage: CodePackageNotFound. CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
                codePackageName_,
                codePackageContext_,
                appHostContext_,
                operationId_);

            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }
        else if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(ApplicationHostAbortCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
            codePackageName_,
            codePackageContext_,
            appHostContext_,
            operationId_);

        codePackage_->AbortAndWaitForTerminationAsync(
            [this](AsyncOperationSPtr const & abortOperation)
            {
                WriteInfo(
                    TraceType,
                    owner_.TraceId,
                    "End(ApplicationHostAbortCodePackage): CPName={0}, CPContext={1}, AppHostContext={2}, OperationId={3}.",
                    codePackageName_,
                    codePackageContext_,
                    appHostContext_,
                    operationId_);

                this->RemoveCodePackage();
                this->TryComplete(abortOperation->Parent);
            },
            thisSPtr);
    }

    void OnCompleted()
    {
        owner_.operationTracker_->Untrack(operationId_);
    }

private:
    ErrorCode TryGetCodePackage()
    {
        AcquireReadLock(owner_.Lock);

        auto iter = owner_.activeCodePackages_.find(codePackageName_);
        if (iter != owner_.activeCodePackages_.end())
        {
            codePackage_ = iter->second;
            return ErrorCode::Success();
        }

        return ErrorCode(ErrorCodeValue::CodePackageNotFound);
    }

    void RemoveCodePackage()
    {
        AcquireWriteLock(owner_.Lock);
        owner_.activeCodePackages_.erase(codePackageName_);
    }

private:
    VersionedServicePackage & owner_;
    wstring codePackageName_;
    ApplicationHostContext  appHostContext_;
    CodePackageContext codePackageContext_;
    TimeSpan timeout_;
    CodePackageSPtr codePackage_;
    uint64 operationId_;
};

class VersionedServicePackage::ApplicationHostCodePackagesAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ApplicationHostCodePackagesAsyncOperation)

public:
    ApplicationHostCodePackagesAsyncOperation(
        _In_ VersionedServicePackage & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        int64 requestorInstanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
        , requestorInstanceId_(requestorInstanceId)
        , codePackageNames_()
        , timeoutHelper_(TimeSpan::FromTicks(request.TimeoutInTicks))
        , lock_()
        , lastError_(ErrorCodeValue::Success)
        , pendingOperationCount_(0)
        , retryDelay_(TimeSpan::FromMilliseconds(500))
        , retryCount_(0)
        , maxRetryDelay_(TimeSpan::FromSeconds(5))
    {
    }

    virtual ~ApplicationHostCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "ApplicationHostCodePackageAsyncOperation: Processing {0}.",
            request_);

        this->StartCodePackageOperations(thisSPtr);
    }

private:
    void StartCodePackageOperations(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToModifying();
        if (!error.IsSuccess())
        {
            this->RetryIfNeeded(thisSPtr);
            return;
        }

        error = this->ValidateOperation();
        if (!error.IsSuccess())
        {
            this->TransitionToOpened(thisSPtr);
            return;
        }

        this->PerformCodePackageOperations(thisSPtr);
    }

    void PerformCodePackageOperations(AsyncOperationSPtr const & thisSPtr)
    {
        this->EnsureCodePackages();

        pendingOperationCount_.store(codePackageNames_.size());

        for (auto codePackageName : codePackageNames_)
        {
            auto timeout = timeoutHelper_.GetRemainingTime();

            auto operation = BeginCodePackageOperation(
                codePackageName,
                [this, codePackageName](AsyncOperationSPtr const & operation)
                {
                    this->FinishCodePackageOperation(operation, false);
                },
                thisSPtr);

            this->FinishCodePackageOperation(operation, true);
        }

        CheckPendingOperations(thisSPtr, codePackageNames_.size());
    }

    void FinishCodePackageOperation(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = this->EndCodePackageOperation(operation);
        if (!error.IsSuccess())
        {
            AcquireWriteLock lock(lock_);
            lastError_.Overwrite(error);
        }

        auto pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    AsyncOperationSPtr BeginCodePackageOperation(
        wstring const & codePackageName,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        AsyncOperationSPtr operation;

        if (request_.IsActivateOperation)
        {
            operation = AsyncOperation::CreateAndStart<ApplicationHostActivateCodePackageAsyncOperation>(
                owner_,
                codePackageName,
                request_.EnvironmentBlock,
                request_.HostContext,
                request_.CodeContext,
                timeoutHelper_.GetRemainingTime(),
                callback,
                parent);
        }
        else if (request_.IsDeactivateOperation)
        {
            operation = AsyncOperation::CreateAndStart<ApplicationHostDeactivateCodePackageAsyncOperation>(
                owner_,
                codePackageName,
                request_.HostContext,
                request_.CodeContext,
                timeoutHelper_.GetRemainingTime(),
                callback,
                parent);
        }
        else if (request_.IsAbortOperation)
        {
            operation = AsyncOperation::CreateAndStart<ApplicationHostAbortCodePackageAsyncOperation>(
                owner_,
                codePackageName,
                request_.HostContext,
                request_.CodeContext,
                timeoutHelper_.GetRemainingTime(),
                callback,
                parent);
        }

        ASSERT_IF(
            operation == nullptr,
            "Invalid ApplicationHostCodePackageOperationType {0}.",
            request_.OperationType);

        return operation;
    }

    ErrorCode EndCodePackageOperation(
        AsyncOperationSPtr const & operation)
    {
        ErrorCode error;

        if (request_.IsActivateOperation)
        {
            error = ApplicationHostActivateCodePackageAsyncOperation::End(operation);
        }
        else if (request_.IsDeactivateOperation)
        {
            error = ApplicationHostDeactivateCodePackageAsyncOperation::End(operation);
        }
        else if (request_.IsAbortOperation)
        {
            error = ApplicationHostAbortCodePackageAsyncOperation::End(operation);
        }

        ASSERT_IF(
            operation == nullptr,
            "Invalid ApplicationHostCodePackageOperationType {0}.",
            request_.OperationType);

        return error;
    }

    void CheckPendingOperations(
        AsyncOperationSPtr const & thisSPtr,
        uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            this->TransitionToOpened(thisSPtr);
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::OperationsPending))
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageAsyncOperation: Failed to transition to Opened state while processing {0}.",
                request_);

            TryComplete(thisSPtr, error);
            return;
        }

        TryComplete(thisSPtr, lastError_);
        return;
    }

    ErrorCode ValidateOperation()
    {
        auto currentInstanceId = owner_.GetActivatorCodePackageInstanceId();
        if (currentInstanceId != requestorInstanceId_)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageAsyncOperation: Activator CP InstanceId={0} mismatched with {0}.",
                currentInstanceId,
                request_);

            return ErrorCode(ErrorCodeValue::InstanceIdMismatch);
        }

        return ErrorCode::Success();
    }

    void EnsureCodePackages()
    {
        if (request_.IsAllCodePackageOperation == false)
        {
            codePackageNames_ = request_.CodePackageNames;
            return;
        }

        for (auto const & dcpDesc : owner_.packageDescription__.DigestedCodePackages)
        {
            if (dcpDesc.Name == owner_.activatorCodePackageName_)
            {
                continue;
            }

            codePackageNames_.push_back(dcpDesc.Name);
        }
    }

    void RetryIfNeeded(AsyncOperationSPtr const & thisSPtr)
    {
        auto currentState = owner_.GetState();
        if (currentState == VersionedServicePackage::Aborted ||
            currentState >= VersionedServicePackage::Closing)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackagesAsyncOperation: Ignoring {0} as current state is '{1}'.",
                request_,
                owner_.StateToString(currentState));

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        if (timeoutHelper_.GetRemainingTime().Ticks > 0)
        {
            retryDelay_ = TimeSpan::FromTicks(++retryCount_ * retryDelay_.Ticks);
            if (retryDelay_ > maxRetryDelay_)
            {
                retryDelay_ = maxRetryDelay_;
            }

            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageAsyncOperation: Retrying {0}. CurrentState={1}, RetryCount={2}, RetryDelay={3}.",
                request_,
                owner_.StateToString(currentState),
                retryCount_,
                retryDelay_);

            Threadpool::Post(
                [this, thisSPtr]() 
                {
                    this->StartCodePackageOperations(thisSPtr);
                },
                retryDelay_);

            return;
        }

        WriteWarning(
            TraceType,
            owner_.TraceId,
            "ApplicationHostCodePackagesAsyncOperation: {0} timed out. CurrentState={1}, RetryCount={2}.",
            request_,
            owner_.StateToString(currentState),
            retryCount_);

        this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));
    }

private:
    VersionedServicePackage & owner_;
    ApplicationHostCodePackageOperationRequest request_;
    int64 requestorInstanceId_;
    vector<wstring> codePackageNames_;
    TimeoutHelper timeoutHelper_;
    RwLock lock_;
    ErrorCode lastError_;
    atomic_uint64 pendingOperationCount_;
    TimeSpan retryDelay_;
    uint64 retryCount_;
    TimeSpan maxRetryDelay_;
};

// ********************************************************************************************************************
// VersionedServicePackage::CodePackagesAsyncOperationBase Implementation
//
class VersionedServicePackage::CodePackagesAsyncOperationBase
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CodePackagesAsyncOperationBase)

public:
    CodePackagesAsyncOperationBase(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        codePackages_(codePackages),
        failed_(codePackages),
        completed_(),
        lock_(),
        lastError_(ErrorCodeValue::Success),
        pendingOperationCount_(0)
    {
    }

    virtual ~CodePackagesAsyncOperationBase()
    {
    }

protected:
    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        auto thisPtr = AsyncOperation::End<CodePackagesAsyncOperationBase>(operation);

        {
            AcquireWriteLock lock(thisPtr->lock_);
            failed = move(thisPtr->failed_);
            completed = move(thisPtr->completed_);
        }

        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation) = 0;

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        pendingOperationCount_.store(codePackages_.size());

        for (auto iter = codePackages_.begin(); iter != codePackages_.end(); ++iter)
        {
            auto codePackage = (*iter).second;
            auto timeout = timeoutHelper_.GetRemainingTime();

            auto operation = BeginCodePackageOperation(
                codePackage,
                timeout,
                [this, codePackage](AsyncOperationSPtr const & operation)
            {
                this->FinishCodePackageOperation(codePackage, operation, false);
            },
                thisSPtr);
            FinishCodePackageOperation(codePackage, operation, true);
        }

        CheckPendingOperations(thisSPtr, codePackages_.size());
    }

    void FinishCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = this->EndCodePackageOperation(codePackage, operation);
        {
            AcquireWriteLock lock(lock_);

            if (!error.IsSuccess())
            {
                lastError_.Overwrite(error);
            }
            else
            {
                auto iter = failed_.find(codePackage->Description.Name);

                ASSERT_IF(
                    iter == failed_.end(),
                    "CodePackage {0} must be present in the failed map as failed map is initialized with all code packages.",
                    codePackage->Description.Name);

                failed_.erase(iter);
                completed_.insert(make_pair(codePackage->Description.Name, codePackage));
            }
        }

        uint64 pendingOperationCount = --pendingOperationCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            auto lastError = ErrorCode(ErrorCodeValue::Success);
            {
                AcquireReadLock lock(lock_);
                lastError = lastError_;
                lastError_.ReadValue();
            }

            TryComplete(thisSPtr, lastError);
            return;
        }
    }

protected:
    VersionedServicePackage & owner_;
    RwLock lock_;

private:
    CodePackageMap const codePackages_;
    TimeoutHelper timeoutHelper_;
    CodePackageMap failed_;
    CodePackageMap completed_;
    ErrorCode lastError_;
    atomic_uint64 pendingOperationCount_;
};

// ********************************************************************************************************************
// VersionedServicePackage::ActivateCodePackageAsyncOperation Implementation
//
class VersionedServicePackage::ActivateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(ActivateCodePackagesAsyncOperation)

public:
    ActivateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~ActivateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ActivateCodePackage): CodePackage={0}:{1} Version={2} Timeout={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            timeout);
        return codePackage->BeginActivate(timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        auto error = codePackage->EndActivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ActivateCodePackage): CodePackage={0}:{1} Version={2} Error={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            error);
        return error;
    }
};

// ********************************************************************************************************************
// VersionedServicePackage::UpdateCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::UpdateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(UpdateCodePackagesAsyncOperation)

public:
    UpdateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        ServicePackageDescription const & newPackageDescription,
        ServicePackageVersionInstance const & newVersionInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent),
        newPackageDescription_(newPackageDescription),
        newVersionInstance_(newVersionInstance)
    {
    }

    virtual ~UpdateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(UpdateCodePackageContext): CodePackage={0}:{1} Version={2}, NewServicePackageVersionInstance={3}, Timeout={4}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            newVersionInstance_,
            timeout);

        // Computing the new RolloutVersion of the CodePackaget to Update.
        // During rollback, it is possible that the RolloutVersion is changed, but the actual content hasn't changed.
        // We do not restart the CodePackage but just update the CodePackate with the new CodePackage and send an update
        // to all Hosts
        RolloutVersion newRolloutVersion = RolloutVersion::Invalid;
        for (auto iter = newPackageDescription_.DigestedCodePackages.begin(); iter != newPackageDescription_.DigestedCodePackages.end(); ++iter)
        {
            if (iter->CodePackage == codePackage->Description.CodePackage)
            {
                newRolloutVersion = iter->RolloutVersionValue;
            }
        }

        //Skip BlockStoreService and ImplicitHost code packages as they are not part of the digested set.
        if (!StringUtility::AreEqualCaseInsensitive(codePackage->Description.CodePackage.Name, (wstring)Constants::ImplicitTypeHostCodePackageName) && !StringUtility::AreEqualCaseInsensitive(codePackage->Description.CodePackage.Name, (wstring)Constants::BlockStoreServiceCodePackageName))
        {
            ASSERT_IF(newRolloutVersion == RolloutVersion::Invalid, "The CodePackage that needs to be updated should be present in the NewPackageDescription. CodePackage={0}", codePackage->Description);
        }

        return codePackage->BeginUpdateContext(newRolloutVersion, newVersionInstance_, timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        auto error = codePackage->EndUpdateContext(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(UpdateCodePackageContext): CodePackage={0}:{1} Version={2}, NewServicePackageVersionInstance={3}, Error={4}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            newVersionInstance_,
            error);
        return error;
    }

private:
    ServicePackageVersionInstance newVersionInstance_;
    ServicePackageDescription newPackageDescription_;
};

// ********************************************************************************************************************
// VersionedServicePackage::DeactivateCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::DeactivateCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(DeactivateCodePackagesAsyncOperation)

public:
    DeactivateCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~DeactivateCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DectivateCodePackage): CodePackage={0}:{1} Version={2} Timeout={3}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version,
            timeout);
        return codePackage->BeginDeactivate(timeout, callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        codePackage->EndDeactivate(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(DectivateCodePackage): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);
        return ErrorCode(ErrorCodeValue::Success);
    }
};

// ********************************************************************************************************************
// VersionedServicePackage::AbortCodePackagesAsyncOperation Implementation
//
class VersionedServicePackage::AbortCodePackagesAsyncOperation
    : public VersionedServicePackage::CodePackagesAsyncOperationBase
{
    DENY_COPY(AbortCodePackagesAsyncOperation)

public:
    AbortCodePackagesAsyncOperation(
        __in VersionedServicePackage & owner,
        CodePackageMap const & codePackages,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : CodePackagesAsyncOperationBase(owner, codePackages, timeout, callback, parent)
    {
    }

    virtual ~AbortCodePackagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageMap & failed, CodePackageMap & completed)
    {
        return CodePackagesAsyncOperationBase::End(operation, failed, completed);
    }

protected:
    AsyncOperationSPtr BeginCodePackageOperation(
        CodePackageSPtr const & codePackage,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {

        UNREFERENCED_PARAMETER(timeout);

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(AbortAndWaitForTermination): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);

        //codePackage->Abort(); // TODO: This should not be called.
        
        return codePackage->BeginAbortAndWaitForTermination(callback, parent);
    }

    ErrorCode EndCodePackageOperation(
        CodePackageSPtr const & codePackage,
        AsyncOperationSPtr const & operation)
    {
        codePackage->EndAbortAndWaitForTermination(operation);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "End(AbortAndWaitForTermination): CodePackage={0}:{1} Version={2}",
            codePackage->CodePackageInstanceId,
            codePackage->InstanceId,
            codePackage->Version);
        return ErrorCode(ErrorCodeValue::Success);
    }
};

class VersionedServicePackage::ActivatorCodePackageTerminatedAsyncOperation
    : public AsyncOperation
    , protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivatorCodePackageTerminatedAsyncOperation)

public:
    ActivatorCodePackageTerminatedAsyncOperation(
        _In_ VersionedServicePackage & owner,
        int64 terminatedInstanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , terminatedInstanceId_(terminatedInstanceId)
        , depdendentCodePackages_()
        , retryDelay_(TimeSpan::FromMilliseconds(500))
        , retryCount_(0)
        , maxRetryDelay_(TimeSpan::FromSeconds(5))
    {
    }

    virtual ~ActivatorCodePackageTerminatedAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivatorCodePackageTerminatedAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        StartProcessingTermination(thisSPtr);
    }

private:
    void StartProcessingTermination(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToModifying();
        if (!error.IsSuccess())
        {
            this->RetryIfNeeded(thisSPtr, error);
            return;
        }

        error = this->ValidateOperation();
        if (!error.IsSuccess())
        {
            this->TransitionToOpened(thisSPtr);
            return;
        }

        auto operation = owner_.operationTracker_->BeginCancelPendingOperationsAndWaitForDrain(
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDrainPendingOperations(operation, false);
            },
            thisSPtr);

        this->FinishDrainPendingOperations(operation, true);
    }

    void FinishDrainPendingOperations(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        owner_.operationTracker_->EndCancelPendingOperationsAndWaitForDrain(operation);

        this->IntializeDependentCodePackagesToAbort();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "ActivatorCodePackageTerminatedAsyncOperation: Begin abort '{0}' dependent CPs. ActivatorInstanceId={1}.",
            depdendentCodePackages_.size(),
            terminatedInstanceId_);

        auto abortOperation = AsyncOperation::CreateAndStart<AbortCodePackagesAsyncOperation>(
            owner_,
            depdendentCodePackages_,
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishAbortCodePackages(operation, false);
            },
            operation->Parent);

        this->FinishAbortCodePackages(abortOperation, true);
    }

    void FinishAbortCodePackages(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        CodePackageMap failedIgnore;
        CodePackageMap completedIgnore;
        AbortCodePackagesAsyncOperation::End(operation, failedIgnore, completedIgnore).ReadValue();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "ActivatorCodePackageTerminatedAsyncOperation: End abort '{0}' dependent CPs. ActivatorInstanceId={1}.",
            depdendentCodePackages_.size(),
            terminatedInstanceId_);

        this->TransitionToOpened(operation->Parent);
    }

    void IntializeDependentCodePackagesToAbort()
    {
        CodePackageSPtr activatorCodePackage;

        {
            AcquireReadLock readLock(owner_.Lock);

            for (auto const & kvPair : owner_.activeCodePackages_)
            {
                if (kvPair.first == owner_.activatorCodePackageName_)
                {
                    activatorCodePackage = kvPair.second;
                    continue;
                }

                depdendentCodePackages_.insert(kvPair);
            }
        }

        ASSERT_IFNOT(
            activatorCodePackage != nullptr,
            "ActivatorCodePackageTerminatedAsyncOperation: Activator CP must be present.");

        {
            AcquireWriteLock writeLock(owner_.Lock);

            owner_.activeCodePackages_.clear();
            owner_.activeCodePackages_.insert(
                make_pair(owner_.activatorCodePackageName_, activatorCodePackage));
        }
    }

    void RetryIfNeeded(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        auto currentState = owner_.GetState();
        if (currentState == VersionedServicePackage::Aborted ||
            currentState >= VersionedServicePackage::Closing)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ActivatorCodePackageTerminatedAsyncOperation: Ignoring as current state is '{0}'.",
                owner_.StateToString(currentState));

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        retryDelay_ = TimeSpan::FromTicks(++retryCount_ * retryDelay_.Ticks);
        if (retryDelay_ > maxRetryDelay_)
        {
            retryDelay_ = maxRetryDelay_;
        }

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "ActivatorCodePackageTerminatedAsyncOperation: Retrying. CurrentState={0}, RetryCount={1}, RetryDelay={2}.",
            owner_.StateToString(currentState),
            retryCount_,
            retryDelay_);

        Threadpool::Post(
            [this, thisSPtr]()
            {
                this->StartProcessingTermination(thisSPtr);
            },
            retryDelay_);

        return;
    }

    ErrorCode ValidateOperation()
    {
        AcquireWriteLock writeLock(owner_.Lock);

        if (owner_.activatorCodePackageInstanceId_ != terminatedInstanceId_)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageAsyncOperation: Terminated activator CP InstanceId={0} mismatched with {0}.",
                terminatedInstanceId_,
                owner_.activatorCodePackageInstanceId_);

            return ErrorCode(ErrorCodeValue::InstanceIdMismatch);
        }

        //
        // Reset to zero so that any new operation request that
        // were initiated by this activator CP are not accepted.
        //
        owner_.activatorCodePackageInstanceId_ = 0;

        return ErrorCode::Success();
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::OperationsPending))
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "ActivatorCodePackageTerminatedAsyncOperation: Failed to transition to Opened state.");

            this->TryComplete(thisSPtr, error);
            return;
        }

        this->TryComplete(thisSPtr, ErrorCode::Success());
    }

private:
    VersionedServicePackage & owner_;
    int64 terminatedInstanceId_;
    CodePackageMap depdendentCodePackages_;
    TimeSpan retryDelay_;
    uint64 retryCount_;
    TimeSpan maxRetryDelay_;
};

// ********************************************************************************************************************
// VersionedServicePackage::OpenAsyncOperation Implementation
//
class VersionedServicePackage::OpenAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in VersionedServicePackage & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        packageVersionInstance_(),
        packageDescription_(),
        packageEnvironment_(),
        codePackages_()
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.GetState() == VersionedServicePackage::Opened)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        auto error = owner_.TransitionToOpening();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        {
            AcquireReadLock lock(owner_.Lock);
            packageVersionInstance_ = owner_.currentVersionInstance__;
            packageDescription_ = owner_.packageDescription__;
        }

        RegisterToLocalResourceManager(thisSPtr);
    }

private:
    void RegisterToLocalResourceManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ServicePackageObj.HostingHolder.RootedObject.LocalResourceManagerObj->RegisterServicePackage(owner_);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        RegisterToHealthManager(thisSPtr);
    }

    void RegisterToHealthManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.HealthManagerObj->RegisterSource(
            owner_.ServicePackageObj.Id,
            owner_.ServicePackageObj.ApplicationName,
            owner_.healthPropertyId_);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        SetupEnvironment(thisSPtr);
    }

    void SetupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(SetupPackageEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeout);
        auto operation = owner_.EnvironmentManagerObj->BeginSetupServicePackageInstanceEnvironment(
            owner_.ServicePackageObj.ApplicationEnvironment,
            owner_.ServicePackageObj.ApplicationName,
            owner_.ServicePackageObj.Id,
            owner_.ServicePackageObj.InstanceId,
            packageDescription_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishSetupEnvironment(operation, false); },
            thisSPtr);
        FinishSetupEnvironment(operation, true);
    }

    void FinishSetupEnvironment(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndSetupServicePackageInstanceEnvironment(operation, packageEnvironment_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(SetupPackageEnvironment): Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
            return;
        }

        // If the ServicePackage being activated is FileStoreService, then setup staging and store shares
        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            owner_.ServicePackageObj.Id.ServicePackageName == L"FileStoreService")
        {
            SetupSMBSharesForFileStoreService(operation->Parent);
        }
        else if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            ConfigureNodeForDnsService(operation->Parent);
        }
        else
        {
            WriteCurrentPackage(operation->Parent);
        }
    }

    void SetupSMBSharesForFileStoreService(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> sidStrings;
        if (Management::FileStoreService::Utility::HasAccountsConfigured())
        {
            auto groupName = AccountHelper::GetAccountName(
                Management::FileStoreService::Constants::FileStoreServiceUserGroup,
                owner_.ServicePackageObj.Id.ApplicationId.ApplicationNumber);

            SidSPtr groupSid;
            auto error = BufferedSid::CreateSPtr(groupName, groupSid);
            ASSERT_IF(error.IsError(ErrorCodeValue::NotFound), "Expected local group name {0} not found", groupName);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to create Sid for FileStoreService group with {0}. Id={1}, Version={2}",
                    error,
                    owner_.ServicePackageObj.Id,
                    packageVersionInstance_);

                TransitionToFailed(thisSPtr, error);
                return;
            }

            wstring sidStr;
            error = groupSid->ToString(sidStr);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to convert Sid to String for FileStoreService group with {0}. Id={1}, Version={2}",
                    error,
                    owner_.ServicePackageObj.Id,
                    packageVersionInstance_);
                TransitionToFailed(thisSPtr, error);
                return;
            }

            sidStrings.push_back(sidStr);
        }
        else
        {

            // If no account is provided in FileStoreService section,
            // then by default ACL shares to current user of fabric
            sidStrings.push_back(owner_.EnvironmentManagerObj->CurrentUserSid);
        }

        if (Management::FileStoreService::FileStoreServiceConfig::GetConfig().AnonymousAccessEnabled)
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Anonymous access has been enabled for FileStoreService. Id={0}, Version={1}",
                owner_.ServicePackageObj.Id,
                packageVersionInstance_);

            SidSPtr anonymousSid, everyoneSid;
            auto error = BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE::WinAnonymousSid, anonymousSid);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            error = BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE::WinWorldSid, everyoneSid);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            wstring anonymousSidStr, everyoneSidStr;
            error = anonymousSid->ToString(anonymousSidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            error = everyoneSid->ToString(everyoneSidStr);
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr, error);
                return;
            }

            sidStrings.push_back(anonymousSidStr);
            sidStrings.push_back(everyoneSidStr);
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ConfigureSMBShare for Store): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeoutHelper_.GetRemainingTime());

        wstring storeLocalFullPath = Path::Combine(
            owner_.ServicePackageObj.runLayout_.GetApplicationWorkFolder(owner_.ServicePackageObj.Id.ApplicationId.ToString()),
            Management::FileStoreService::Constants::StoreRootDirectoryName);
        wstring storeShareName = wformatString("{0}_{1}", Management::FileStoreService::Constants::StoreShareName, owner_.ServicePackageObj.HostingHolder.Value.NodeName);
        auto operation = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->BeginConfigureSMBShare(
            sidStrings,
            GENERIC_READ | GENERIC_EXECUTE,
            storeLocalFullPath,
            storeShareName,
            timeoutHelper_.GetRemainingTime(),
            [this, sidStrings](AsyncOperationSPtr const & operation) { this->FinishConfigureStoreShare(operation, false, sidStrings); },
            thisSPtr);
        FinishConfigureStoreShare(operation, true, sidStrings);
    }

    void FinishConfigureStoreShare(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously, vector<wstring> const & sidStrings)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        AsyncOperationSPtr thisSPtr = operation->Parent;
        auto error = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->EndConfigureSMBShare(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ConfigureSMBShare for Store): Id={0}, Version={1}, Error: {2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ConfigureSMBShare for Staging): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeoutHelper_.GetRemainingTime());

        wstring stagingLocalFullPath = Path::Combine(
            owner_.ServicePackageObj.runLayout_.GetApplicationWorkFolder(owner_.ServicePackageObj.Id.ApplicationId.ToString()),
            Management::FileStoreService::Constants::StagingRootDirectoryName);
        wstring stagingShareName = wformatString("{0}_{1}", Management::FileStoreService::Constants::StagingShareName, owner_.ServicePackageObj.HostingHolder.Value.NodeName);
        auto configureOperation = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->BeginConfigureSMBShare(
            sidStrings,
            GENERIC_ALL,
            stagingLocalFullPath,
            stagingShareName,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishConfigureStagingShare(operation, false); },
            thisSPtr);
        FinishConfigureStagingShare(configureOperation, true);
    }

    void FinishConfigureStagingShare(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        AsyncOperationSPtr thisSPtr = operation->Parent;
        auto error = owner_.ServicePackageObj.HostingHolder.Value.FabricActivatorClientObj->EndConfigureSMBShare(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ConfigureSMBShare for Staging): Id={0}, Version={1}, Error: {2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        WriteCurrentPackage(thisSPtr);
    }

    void ConfigureNodeForDnsService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->BeginSetupDnsEnvAndStartMonitor(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceComplete(operation, false /*expectedCompletedSynchronously*/); },
            thisSPtr);

        this->OnConfigureNodeForDnsServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnConfigureNodeForDnsServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->EndSetupDnsEnvAndStartMonitor(operation);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.TraceId,
                "Failed to configure node for DNS service, error {0}", error);
        }
        else
        {
            WriteInfo(TraceType, owner_.TraceId,
                "Successfully configured node for DNS service");
        }

        // Always continue to next step, regardless of the return value.
        // Error in environment setup should not prevent DnsService from being started.
        // DnsService environment is needed for regular services only, containers can still function properly.
        // DnsService will display warning on the node which is not configured properly.
        WriteCurrentPackage(thisSPtr);
    }

    void WriteCurrentPackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.WriteCurrentPackageFile(packageVersionInstance_.Version);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "WriteCurrentPackageFile: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        AddServiceTypesEntriesToServiceTypeStateManager(thisSPtr);
    }

    void AddServiceTypesEntriesToServiceTypeStateManager(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServicePackageActivated(
            owner_.serviceTypeInstanceIds_,
            owner_.ServicePackageObj.ApplicationName,
            owner_.componentId_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnServicePackageActivated: Id={0}, Version={1}, ErrorCode={2} ",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        owner_.registrationTimeoutTracker_->Reset(owner_.serviceTypeInstanceIds_);

        LoadCodePackages(thisSPtr);
    }

    void LoadCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.LoadCodePackages(
            packageVersionInstance_,
            packageDescription_,
            packageEnvironment_,
            owner_.isOnDemandActivationEnabled_,
            codePackages_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "LoadCodePackages: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        owner_.ActivateCodePackagesAsync(
            codePackages_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishActivateCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
    }

    void FinishActivateCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(failed);     // in case of failure we abort all of the codePackages_
        UNREFERENCED_PARAMETER(completed);  // in case of success we move all of the codePackages_ inside the package

        if (!error.IsSuccess())
        {
            TransitionToFailed(operation->Parent, error);
            return;
        }
        else
        {
            {
                AcquireWriteLock lock(owner_.Lock);
                owner_.environmentContext__ = packageEnvironment_;
                owner_.activeCodePackages_ = codePackages_;
            }

            TransitionToOpened(operation->Parent);
            return;
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.HealthManagerObj->ReportHealth(
            owner_.ServicePackageObj.Id,
            owner_.healthPropertyId_,
            SystemHealthReportCode::Hosting_ServicePackageActivated,
            L"" /*extraDescription*/,
            SequenceNumber::GetNext());

        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failureError)
    {
        owner_.AbortCodePackagesAsync(
            codePackages_,
            [this, failureError](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            AbortPackageEnvironment(operation->Parent, failureError);
        },
            thisSPtr);
    }

    void AbortPackageEnvironment(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        if (packageEnvironment_)
        {
            WriteInfo(
                TraceType, owner_.TraceId,
                "VersionedServicePackage.OpenAsyncOperation(): Cleaning up Environmnent ServicePackage: {0} {1}", owner_.ServicePackageObj.Id, owner_.ServicePackageObj.InstanceId);

            owner_.EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(
                packageEnvironment_,
                packageDescription_,
                owner_.ServicePackageObj.InstanceId);
        }

        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->RemoveDnsFromEnvAndStopMonitorAndWait();
        }

        FinishTransitionToFailed(thisSPtr, error);
    }

    void FinishTransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        owner_.TransitionToFailed().ReadValue();
        TryComplete(thisSPtr, error);
        return;
    }

private:
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance packageVersionInstance_;
    ServicePackageDescription packageDescription_;
    ServicePackageInstanceEnvironmentContextSPtr packageEnvironment_;
    CodePackageMap codePackages_;
};

// ********************************************************************************************************************
// VersionedServicePackage::SwitchAsyncOperation Implementation
//
class VersionedServicePackage::SwitchAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SwitchAsyncOperation)

public:
    SwitchAsyncOperation(
        __in VersionedServicePackage & owner,
        ServicePackageVersionInstance const & toVersionInstance,
        ServicePackageDescription const & newPackageDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        currentVersionInstance_(),
        currentPackageDescription_(),
        newVersionInstance_(toVersionInstance),
        newPackageDescription_(newPackageDescription),
        newCodePackages_(),
        codePackagesToActivate_(),
        codePackagesToDeactivate_(),
        codePackagesToUpdate_()
    {
    }

    virtual ~SwitchAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SwitchAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToSwitching();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        //For Testing. This test config cancels the parent UpgradeService Package Operation 
        if (HostingConfig::GetConfig().AbortSwitchServicePackage)
        {
            thisSPtr->Parent->Cancel();
            return;
        }

        owner_.GetCurrentVersionInstanceAndPackageDescription(
            currentVersionInstance_,
            currentPackageDescription_);

        // check if version instances are same
        if (currentVersionInstance_ == newVersionInstance_)
        {
            TransitionToOpened(thisSPtr);
            return;
        }

        bool versionUpdateOnly;
        this->ValidateSwitchOperation(versionUpdateOnly);

        if (versionUpdateOnly)
        {
            {
                AcquireReadLock lock(owner_.Lock);
                codePackagesToUpdate_ = owner_.activeCodePackages_;
                newCodePackages_ = owner_.activeCodePackages_;
            }

            SwitchCurrentPackageFile(thisSPtr);
        }
        else
        {
            // Reset the activation time during switch so that we do not report
            // ServiceTypeRegistration timeout without waiting for sufficient time
            owner_.registrationTimeoutTracker_->ResetTrackingTime();

            this->LoadNewCodePackages(thisSPtr);
        }
    }

private:
    void LoadNewCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.LoadCodePackages(
            newVersionInstance_, 
            newPackageDescription_, 
            owner_.environmentContext__, 
            false /* onlyActivatorCodePackage */,
            newCodePackages_);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "LoadCodePackages: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            newVersionInstance_,
            error);

        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        SwitchCodePackages(thisSPtr);
    }

    void SwitchCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        //
        // For SP with on-demand CP activation, switch operation is only
        // invoked when new SP has same set of CPs and rollout version of
        // activator CP is same. Only dependent CPs can have their rollout
        // version changed. 
        // 
        {
            AcquireReadLock lock(owner_.Lock);

            for (auto newCp : newCodePackages_)
            {
                auto iterCurrent = owner_.activeCodePackages_.find(newCp.first);
                if (iterCurrent == owner_.activeCodePackages_.end())
                {
                    //
                    // For SP with on demand activation it means the CP has not been
                    // activated by master CP.
                    //
                    if (!owner_.isOnDemandActivationEnabled_)
                    {
                        codePackagesToActivate_.insert(make_pair(newCp.first, newCp.second));
                    }
                }
                else
                {
                    if (newCp.second->RolloutVersionValue != iterCurrent->second->RolloutVersionValue)
                    {
                        codePackagesToDeactivate_.insert(make_pair(iterCurrent->first, iterCurrent->second));
                        codePackagesToActivate_.insert(make_pair(newCp.first, newCp.second));
                    }
                    else
                    {
                        codePackagesToUpdate_.insert(make_pair(iterCurrent->first, iterCurrent->second));
                    }
                }
            }

            if (!owner_.isOnDemandActivationEnabled_)
            {
                for (auto const & kvPair : owner_.activeCodePackages_)
                {
                    auto iter = newCodePackages_.find(kvPair.first);
                    if (iter == newCodePackages_.end())
                    {
                        codePackagesToDeactivate_.insert(make_pair(kvPair.first, kvPair.second));
                    }
                }
            }
        }

        DeactivateModifiedAndRemovedOldCodePackages(thisSPtr);
    }

    void DeactivateModifiedAndRemovedOldCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        bool useActivationTimeout = true;
        if (HostingConfig::GetConfig().ActivationTimeout > HostingConfig::GetConfig().ApplicationUpgradeTimeout)
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "ApplicationUpgradeTimeout {0} should be more than ActivationTimeout {1}.",
                HostingConfig::GetConfig().ApplicationUpgradeTimeout,
                HostingConfig::GetConfig().ActivationTimeout);

            useActivationTimeout = false;
        }

        owner_.DeactivateCodePackagesAsync(
            codePackagesToDeactivate_,
            useActivationTimeout ? HostingConfig::GetConfig().ActivationTimeout : timeoutHelper_.GetRemainingTime(),
            [this](
                AsyncOperationSPtr const & operation, 
                ErrorCode const error, 
                CodePackageMap const & completed, 
                CodePackageMap const & failed)
            {
                this->FinishDeactivateModifiedAndRemovedOldCodePackages(operation, error, completed, failed);
            },
            thisSPtr);
        return;
    }

    void FinishDeactivateModifiedAndRemovedOldCodePackages(
        AsyncOperationSPtr const & operation, 
        ErrorCode const error, 
        CodePackageMap const & completed, 
        CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);

        // abort the failed code packages, the completed ones are already inactive
        if (!error.IsSuccess())
        {
            AbortFailedModifiedAndRemovedOldCodePackages(operation->Parent, failed);
        }
        else
        {
            SwitchCurrentPackageFile(operation->Parent);
        }
    }

    void AbortFailedModifiedAndRemovedOldCodePackages(
        AsyncOperationSPtr const & thisSPtr, 
        CodePackageMap const & failedCodePackages)
    {
        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](
                AsyncOperationSPtr const & operation, 
                ErrorCode const error, 
                CodePackageMap const &, 
                CodePackageMap const &)
            {
                error.ReadValue();
                this->SwitchCurrentPackageFile(operation->Parent);
            },
            thisSPtr);
        return;
    }

    void SwitchCurrentPackageFile(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.WriteCurrentPackageFile(newVersionInstance_.Version);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "SwitchCurrentPackageFile: Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            newVersionInstance_.Version,
            error);
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }

        {
            AcquireWriteLock lock(owner_.Lock);
            owner_.currentVersionInstance__ = newVersionInstance_;
            owner_.packageDescription__ = newPackageDescription_;
        }

        UpdateExistingCodePackages(thisSPtr);
    }

    void UpdateExistingCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.UpdateCodePackagesAsync(
            codePackagesToUpdate_,
            newPackageDescription_,
            newVersionInstance_,
            timeoutHelper_.GetRemainingTime(),
            [this](
                AsyncOperationSPtr const & operation, 
                ErrorCode const error, 
                CodePackageMap const & completed, 
                CodePackageMap const & failed)
            {
                this->FinishUpdateExistingCodePackages(operation, error, completed, failed);
            },
            thisSPtr);
        return;
    }

    void FinishUpdateExistingCodePackages(
        AsyncOperationSPtr const & operation, 
        ErrorCode const error, 
        CodePackageMap const & completed, 
        CodePackageMap const & failed)
    {
        // the completed code packages are updated, updated + activated = new code packages of this service package
        codePackagesToUpdate_ = completed;

        // abort the failed code packages, and add them to activate list
        if (!error.IsSuccess())
        {
            if (owner_.isOnDemandActivationEnabled_ &&
                failed.find(owner_.activatorCodePackageName_) != failed.end())
            {
                // Activator CP failed to update. Abort all the CPs.
                codePackagesToUpdate_.clear();

                {
                    //
                    // Reset activator CP instanceId so that any new operation
                    // request were initiated by this activator CP are not accepted.
                    //
                    AcquireWriteLock writeLock(owner_.Lock);
                    owner_.activatorCodePackageInstanceId_ = 0;
                }

                CodePackageMap codePackagesToAbort = failed;
                for (auto const & cp : completed)
                {
                    codePackagesToAbort.insert(make_pair(cp.first, cp.second));
                }

                AbortUpdateFailedOldCodePackages(operation->Parent, codePackagesToAbort);
            }
            else
            {
                AbortUpdateFailedOldCodePackages(operation->Parent, failed);
            }
        }
        else
        {
            ActivateModifiedAndAddedNewCodePackages(operation->Parent);
        }
    }

    void AbortUpdateFailedOldCodePackages(
        AsyncOperationSPtr const & thisSPtr, 
        CodePackageMap const & failedCodePackages)
    {
        // CPs for which update failed will be aborted and activated again.
        // For case of SP with activator CP, only activator CP will be activated.
        if (owner_.isOnDemandActivationEnabled_)
        {
            auto iter = failedCodePackages.find(owner_.activatorCodePackageName_);
            if (iter != failedCodePackages.end())
            {
                auto newCodePackage = this->CreateCodePackageForNewVersion(iter->second);
                codePackagesToActivate_.insert(
                    make_pair(
                        iter->second->Description.Name,
                        move(newCodePackage)));
            }
        }
        else
        {
            for (auto kvPair : failedCodePackages)
            {
                auto newCodePackage = this->CreateCodePackageForNewVersion(kvPair.second);

                codePackagesToActivate_.insert(
                    make_pair(
                        kvPair.second->Description.Name,
                        move(newCodePackage)));
            }
        }

        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
            {
                error.ReadValue();
                this->ActivateModifiedAndAddedNewCodePackages(operation->Parent);
            },
            thisSPtr);
        return;
    }

    void ActivateModifiedAndAddedNewCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.ActivateCodePackagesAsync(
            codePackagesToActivate_,
            timeoutHelper_.GetRemainingTime(),
            [this](
                AsyncOperationSPtr const & operation, 
                ErrorCode const error, 
                CodePackageMap const & completed, 
                CodePackageMap const & failed)
            {
                this->FinishActivateModifiedAndAddedNewCodePackages(operation, error, completed, failed);
            },
            thisSPtr);
        return;
    }

    void FinishActivateModifiedAndAddedNewCodePackages(
        AsyncOperationSPtr const & operation, 
        ErrorCode const error, 
        CodePackageMap const & completed, 
        CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);
        UNREFERENCED_PARAMETER(failed);

        if (!error.IsSuccess())
        {
            // deactivate all of the packages we tried to update and move to Failed state.
            owner_.AbortCodePackagesAsync(
                codePackagesToActivate_,
                [this](
                    AsyncOperationSPtr const & operation, 
                    ErrorCode const error, 
                    CodePackageMap const &, 
                    CodePackageMap const &)
                {
                    error.ReadValue();
                    this->TransitionToFailed(operation->Parent, error);
                },
                operation->Parent);
        }
        else
        {
            ChangeOwnedCodePackages();
            TransitionToOpened(operation->Parent);
        }
    }

    void ChangeOwnedCodePackages()
    {
        size_t prevActiveCpCount;
        size_t newActiveCpCount;

        {
            AcquireWriteLock lock(owner_.Lock);

            prevActiveCpCount = owner_.activeCodePackages_.size();

            owner_.activeCodePackages_.clear();

            for (auto kvPair : codePackagesToUpdate_)
            {
                owner_.activeCodePackages_.insert(make_pair(kvPair.first, kvPair.second));
            }

            for (auto kvPair : codePackagesToActivate_)
            {
                owner_.activeCodePackages_.insert(make_pair(kvPair.first, kvPair.second));
            }

            newActiveCpCount = owner_.activeCodePackages_.size();
        }

        if (owner_.isOnDemandActivationEnabled_)
        {
            if (newActiveCpCount != 1 && newActiveCpCount != prevActiveCpCount)
            {
                TRACE_ERROR_AND_ASSERT(
                    TraceType,
                    "Final CP size {0} must match previous CP size {0} or it should be 1 when OnDemandActivation is enabled.",
                    newActiveCpCount,
                    prevActiveCpCount);
            }
        }
        else if(newActiveCpCount != newCodePackages_.size())
        {
            TRACE_ERROR_AND_ASSERT(
                TraceType,
                "The size {0} of the final code packages must match size {0} of new code packages",
                newActiveCpCount,
                newCodePackages_.size());
        }
    }

    void TransitionToOpened(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToOpened();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr, error);
            return;
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr, ErrorCode const failedError)
    {
        CodePackageMap existingCodePackages;
        {
            AcquireWriteLock lock(owner_.Lock);
            existingCodePackages = move(owner_.activeCodePackages_);
            owner_.activeCodePackages_.clear();
        }

        owner_.AbortCodePackagesAsync(
            existingCodePackages,
            [this, failedError](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            owner_.TransitionToFailed().ReadValue();
            TryComplete(operation->Parent, failedError);
        },
            thisSPtr);
        return;
    }

    CodePackageSPtr CreateCodePackageForNewVersion(CodePackageSPtr oldVersionCp)
    {
        return make_shared<CodePackage>(
            owner_.ServicePackageObj.HostingHolder,
            VersionedServicePackageHolder(owner_, owner_.CreateComponentRoot()),
            oldVersionCp->Description,
            owner_.ServicePackageObj.Id,
            owner_.ServicePackageObj.InstanceId,
            newVersionInstance_,
            owner_.ServicePackageObj.ApplicationName,
            oldVersionCp->RolloutVersionValue,
            owner_.environmentContext__,
            oldVersionCp->ActivatorCodePackageInstanceId,
            move(EnvironmentMap()),
            oldVersionCp->RunAsId,
            oldVersionCp->SetupRunAs,
            oldVersionCp->IsShared,
            ProcessDebugParameters(),
            oldVersionCp->PortBindings,
            oldVersionCp->RemoveServiceFabricRuntimeAccess,
            oldVersionCp->GuestServiceTypes);
    }

    void ValidateSwitchOperation(_Out_ bool & versionUpdateOnly)
    {
        versionUpdateOnly = (currentPackageDescription_.ContentChecksum == newPackageDescription_.ContentChecksum);
        
        auto currentRolloutVersion = currentVersionInstance_.Version.RolloutVersionValue;
        auto newRolloutVersion = newVersionInstance_.Version.RolloutVersionValue;

        ASSERT_IF(
            currentRolloutVersion.Major != newRolloutVersion.Major && !versionUpdateOnly,
            "Cannot switch major versions. ServicePackageInstanceId={0}, CurrentVersion={1}, NewVersion={2}, VersionUpdateOnly={3}",
            owner_.ServicePackageObj.Id,
            currentVersionInstance_,
            newVersionInstance_,
            versionUpdateOnly);

        if (owner_.isOnDemandActivationEnabled_ && !owner_.isGuestApplication_)
        {
            CODING_ERROR_ASSERT(
                currentPackageDescription_.DigestedCodePackages.size() == newPackageDescription_.DigestedCodePackages.size());

            // TODO: Add mopre validation to make sure our rollout version compute logic 
            // has no bug

            auto hasMatchingActivatorCodePackage = false;

            for (auto dcpDesc : newPackageDescription_.DigestedCodePackages)
            {
                if (StringUtility::AreEqualCaseInsensitive(dcpDesc.CodePackage.Name, owner_.activatorCodePackageName_) &&
                    dcpDesc.CodePackage.IsActivator &&
                    dcpDesc.RolloutVersionValue == owner_.activatorCodePackageRolloutVersion_)
                {
                    hasMatchingActivatorCodePackage = true;
                    break;
                }
            }

            ASSERT_IF(
                !hasMatchingActivatorCodePackage,
                "Invalid switch operation. ServicePackageInstanceId={0}, CurrentVersion={1}, NewVersion={2}, VersionUpdateOnly={3}",
                owner_.ServicePackageObj.Id,
                currentVersionInstance_,
                newVersionInstance_,
                versionUpdateOnly);
        }
    }

private:
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance currentVersionInstance_;
    ServicePackageDescription currentPackageDescription_;
    ServicePackageVersionInstance const newVersionInstance_;
    ServicePackageDescription newPackageDescription_;
    CodePackageMap newCodePackages_;

    CodePackageMap codePackagesToActivate_;
    CodePackageMap codePackagesToDeactivate_;
    CodePackageMap codePackagesToUpdate_;
};

// ********************************************************************************************************************
// VersionedServicePackage::CloseAsyncOperation Implementation
//
class VersionedServicePackage::CloseAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in VersionedServicePackage & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        packageVersionInstance_(),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static void End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.GetState() == VersionedServicePackage::Closed)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        auto error = owner_.TransitionToClosing();
        if (!error.IsSuccess())
        {
            TransitionToAborted(thisSPtr);
            return;
        }
        AsyncOperationSPtr activateOperation;
        AsyncOperationSPtr deactivateOperation;
        {
            AcquireReadLock lock(owner_.Lock);
            packageVersionInstance_ = owner_.currentVersionInstance__;
            activateOperation = move(owner_.eventWaitAsyncOperation_);
            deactivateOperation = move(owner_.deactivateAsyncOperation_);
        }
        if (activateOperation)
        {
            activateOperation->Cancel();
        }
        if (deactivateOperation)
        {
            deactivateOperation->Cancel();
        }
        DeactivateCodePackages(thisSPtr);
    }

private:

    void DeactivateCodePackages(AsyncOperationSPtr const & thisSPtr)
    {
        std::unique_ptr<Common::AsyncAutoResetEvent> activateEventHandler;
        std::unique_ptr<Common::AsyncAutoResetEvent> deactivateEventHandler;
        CodePackageMap existingCodePackages;
        {
            AcquireWriteLock lock(owner_.Lock);
            existingCodePackages = move(owner_.activeCodePackages_);
            owner_.activeCodePackages_.clear();
            activateEventHandler = move(owner_.activateEventHandler_);
            deactivateEventHandler = move(owner_.deactivateEventHandler_);
        }

        if (activateEventHandler)
        {
            //activateEventHandler->Cancel();
        }
        if (deactivateEventHandler)
        {
            //deactivateEventHandler->Cancel();
            existingCodePackages = move(owner_.activeCodePackages_);
            owner_.activeCodePackages_.clear();
        }

        owner_.DeactivateCodePackagesAsync(
            existingCodePackages,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
        {
            this->FinishDeactivateCodePackages(operation, error, completed, failed);
        },
            thisSPtr);
        return;
    }

    void FinishDeactivateCodePackages(AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const & completed, CodePackageMap const & failed)
    {
        UNREFERENCED_PARAMETER(completed);

        if (!error.IsSuccess())
        {
            AbortFailedCodePackages(operation->Parent, failed);
        }
        else
        {
            CleanupEnvironment(operation->Parent);
        }
    }

    void AbortFailedCodePackages(AsyncOperationSPtr const & thisSPtr, CodePackageMap const & failedCodePackages)
    {
        owner_.AbortCodePackagesAsync(
            failedCodePackages,
            [this](AsyncOperationSPtr const & operation, ErrorCode const error, CodePackageMap const &, CodePackageMap const &)
        {
            error.ReadValue();
            this->CleanupEnvironment(operation->Parent);
        },
            thisSPtr);
        return;
    }

    void CleanupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        ServicePackageInstanceEnvironmentContextSPtr packageEnvironment;

        {
            AcquireWriteLock lock(owner_.Lock);
            packageEnvironment = move(owner_.environmentContext__);
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CleanupPackageEnvironment): Id={0}, Version={1}, Timeout={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            timeout);
        auto operation = owner_.EnvironmentManagerObj->BeginCleanupServicePackageInstanceEnvironment(
            packageEnvironment,
            owner_.packageDescription__,
            owner_.ServicePackageObj.InstanceId,
            timeout,
            [this, packageEnvironment](AsyncOperationSPtr const & operation) { this->FinishCleanupEnvironment(packageEnvironment, operation, false); },
            thisSPtr);
        FinishCleanupEnvironment(packageEnvironment, operation, true);
    }

    void FinishCleanupEnvironment(
        ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironment,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.EnvironmentManagerObj->EndCleanupServicePackageInstanceEnvironment(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CleanupPackageEnvironment): Id={0}, Version={1}, ErrorCode={2}",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            error);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType, owner_.TraceId,
                "VersionedServicePackage: Cleaning up Environmnent ServicePackage: {0} {1}", owner_.ServicePackageObj.Id, owner_.ServicePackageObj.InstanceId);

            owner_.EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(
                packageEnvironment,
                owner_.packageDescription__,
                owner_.ServicePackageObj.InstanceId);
        }

        if (owner_.ServicePackageObj.Id.ApplicationId.IsSystem() &&
            StringUtility::AreEqualCaseInsensitive(owner_.ServicePackageObj.Id.ServicePackageName,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
        {
            ConfigureNodeForDnsService(operation->Parent);
        }
        else
        {
            RemoveCurrentPackageFile(operation->Parent);
        }
    }

    void ConfigureNodeForDnsService(AsyncOperationSPtr const & thisSPtr)
    {
        std::wstring sidString;
        const std::wstring DnsServicePackageName(L"Code");
        SecurityPrincipalInformationSPtr info;
        ErrorCode error = owner_.TryGetPackageSecurityPrincipalInfo(DnsServicePackageName, info);
        if (error.IsSuccess())
        {
            sidString = info->SidString;
        }

        auto operation = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->BeginRemoveDnsFromEnvAndStopMonitor(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceComplete(operation, false /*expectedCompletedSynchronously*/); },
            thisSPtr);

        this->OnConfigureNodeForDnsServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnConfigureNodeForDnsServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = owner_.ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->EndRemoveDnsFromEnvAndStopMonitor(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.TraceId,
                "Failed to remove DNS service configuration from node, error {0}", error);
        }
        else
        {
            WriteInfo(TraceType, owner_.TraceId,
                "Successfully removed DNS service configuration from node");
        }

        // Always continue to the next step. Cleanup cannot fail if successfully invoked.
        // The node will continue to function properly even if the environment is not cleaned up properly.
        RemoveCurrentPackageFile(thisSPtr);
    }

    void RemoveCurrentPackageFile(AsyncOperationSPtr const & thisSPtr)
    {
        auto currentPackageFilePath = owner_.runLayout_.GetCurrentServicePackageFile(
            owner_.ServicePackageObj.Id.ApplicationId.ToString(),
            owner_.ServicePackageObj.ServicePackageName,
            owner_.ServicePackageObj.Id.PublicActivationId);

        auto error = File::Delete2(currentPackageFilePath, true);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FileNotFound))
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to remove current package file: Id={0}, Version={1}, File={2}. ErrorCode={3}.",
                owner_.ServicePackageObj.Id,
                packageVersionInstance_,
                currentPackageFilePath,
                error);

            TransitionToFailed(thisSPtr);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Successfully removed current package file: Id={0}, Version={1}, File={2}. ErrorCode={3}.",
            owner_.ServicePackageObj.Id,
            packageVersionInstance_,
            currentPackageFilePath,
            error);

        TransitionToClosed(thisSPtr);
    }

    void TransitionToClosed(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToClosed();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
            return;
        }
        else
        {
            // In every case we need to unregister service package from LRM to free up resources.
            owner_.LocalResourceManagerObj->UnregisterServicePackage(owner_);

            // unregister the componenet from Healthmanager
            owner_.HealthManagerObj->UnregisterSource(owner_.ServicePackageObj.Id, owner_.healthPropertyId_);

            // remove any failures registered for this service package
            owner_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->UnregisterFailure(owner_.failureId_);

            // Deletes all the ServiceType entries belonging to this ServicePackage from the ServiceTypeStateManager
            owner_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->OnServicePackageDeactivated(
                owner_.serviceTypeInstanceIds_,
                owner_.componentId_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.TransitionToFailed().ReadValue();
        TransitionToAborted(thisSPtr);
    }

    void TransitionToAborted(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.AbortAndWaitForTerminationAsync(
            [this](AsyncOperationSPtr const & operation)
        {
            this->TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        },
            thisSPtr);
    }

private:
    VersionedServicePackage & owner_;
    TimeoutHelper timeoutHelper_;
    ServicePackageVersionInstance packageVersionInstance_;
};

// ********************************************************************************************************************
// VersionedServicePackage::TerminateFabricTypeHostAsyncOperation Implementation
//
class VersionedServicePackage::TerminateFabricTypeHostAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(TerminateFabricTypeHostAsyncOperation)

public:
    TerminateFabricTypeHostAsyncOperation(
        __in VersionedServicePackage & owner,
        int64 instanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timer_(),
        instanceId_(instanceId)
    {
    }

    virtual ~TerminateFabricTypeHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out int64 & instanceId)
    {
        auto thisPtr = AsyncOperation::End<TerminateFabricTypeHostAsyncOperation>(operation);
        instanceId = move(thisPtr->instanceId_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TryTerminateFabricTypeHostCodePackage(thisSPtr);
    }

private:
    void TryTerminateFabricTypeHostCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        bool isOpened = CheckIfOpened();

        if (!isOpened)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: VersionedServicePackage is closed");
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        //
        // Trigger failover only if failure threshold is exceeded.
        //
        auto failureCountThreshold = (ULONG)HostingConfig::GetConfig().DeployedServiceFailoverContinuousFailureThreshold;
        if (owner_.GetFailureCount() < failureCountThreshold)
        {
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        CodePackageSPtr fabricTypeHostPackageSPtr;

        {
            AcquireReadLock lock(owner_.Lock);

            auto iter = owner_.activeCodePackages_.find(Constants::ImplicitTypeHostCodePackageName);
            if (iter != owner_.activeCodePackages_.end())
            {
                fabricTypeHostPackageSPtr = iter->second;
            }
        }

        //If the FabricTypeHost code package is not found then we ended up deactivating the code package. So, no need for termination.
        if (!fabricTypeHostPackageSPtr)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: FabricTypeHost code package not hosted");
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        TimeSpan retryDueTime;
        auto error = fabricTypeHostPackageSPtr->TerminateCodePackageExternally(retryDueTime);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "TerminateFabricTypeHostCodePackage: Fabric unable to terminate FabricTypeHost with error {0}",
                error);

            //AppHostId not found at Fabric, will retry
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "TerminateFabricTypeHostCodePackage: Retrying FabricTypeHost termination in {0}ms",
                    retryDueTime.TotalMilliseconds());

                timer_ = Timer::Create("VersionedServicePackage.TerminateFabricTypeHost",
                    [this, thisSPtr](TimerSPtr const & timer)
                {
                    timer->Cancel();
                    this->TryTerminateFabricTypeHostCodePackage(thisSPtr);
                },
                    false);
                timer_->Change(retryDueTime);

                return;
            }
        }

        thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    bool CheckIfOpened()
    {
        return (owner_.GetState() == VersionedServicePackage::Opened);
    }

private:
    VersionedServicePackage & owner_;
    TimerSPtr timer_;
    int64 instanceId_;
};

// ********************************************************************************************************************
// VersionedServicePackage::RestartCodePackageInstanceAsyncOperation Implementation
//
class VersionedServicePackage::RestartCodePackageInstanceAsyncOperation
    : public AsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RestartCodePackageInstanceAsyncOperation)

public:
    RestartCodePackageInstanceAsyncOperation(
        __in VersionedServicePackage & owner,
        wstring const & codePackageName,
        int64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageName_(codePackageName)
        , instanceId_(instanceId)
        , timeout_(timeout)
    {
    }

    virtual ~RestartCodePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RestartCodePackageInstanceAsyncOperation>(operation);
        
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        uint64 currentState;
        auto shouldRestart = this->IsCurrentStateValidForRestart(currentState);

        if (!shouldRestart)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "RestartCodePackageInstanceAsyncOperation: Skipping restart as current VSP state is '{0}'. CodePackageName=[{1}], InstanceId=[{2}].",
                owner_.StateToString(currentState),
                codePackageName_,
                instanceId_);

            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        this->RestartCodePackage(thisSPtr);
    }

private:
    void RestartCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageSPtr codePackageSPtr;

        {
            AcquireReadLock lock(owner_.Lock);

            auto iter = owner_.activeCodePackages_.find(codePackageName_);
            if (iter != owner_.activeCodePackages_.end())
            {
                codePackageSPtr = iter->second;
            }
        }

        // If code package is not found then we ended up deactivating the code package. So, no need for termination.
        if (!codePackageSPtr)
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "RestartCodePackageInstanceAsyncOperation: CodePackage '{0}' is not present.",
                codePackageName_);
            
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::NotFound);
            return;
        }

        auto operation = codePackageSPtr->BeginRestartCodePackageInstance(
            instanceId_,
            timeout_,
            [this, codePackageSPtr](AsyncOperationSPtr const & operation) { OnRestartCompleted(operation, false, codePackageSPtr); },
            thisSPtr);

        this->OnRestartCompleted(operation, true, codePackageSPtr);
    }

    void OnRestartCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, CodePackageSPtr codePackageSPtr)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = codePackageSPtr->EndRestartCodePackageInstance(operation);

        TryComplete(operation->Parent, error);
    }

    bool IsCurrentStateValidForRestart(uint64 & currState) // TODO: Should we include Analyzing state as valid state.
    {
        currState = owner_.GetState();

        return (currState == VersionedServicePackage::Opened);
    }

private:
    VersionedServicePackage & owner_;
    wstring codePackageName_;
    int64 instanceId_;
    TimeSpan timeout_;
};

// ********************************************************************************************************************
// VersionedServicePackage Implementation
//
VersionedServicePackage::VersionedServicePackage(
    ServicePackage2Holder const & servicePackageHolder,
    ServicePackageVersionInstance const & versionInstance,
    ServicePackageDescription const & servicePackageDescription,
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
    : ComponentRoot(),
    StateMachine(Created),
    servicePackageHolder_(servicePackageHolder),
    currentVersionInstance__(versionInstance),
    packageDescription__(servicePackageDescription),
    environmentContext__(),
    activeCodePackages_(),
    failureId_(
        wformatString(
            "VersionedServicePackage:{0}:{1}",
            servicePackageHolder.RootedObject.Id, 
            servicePackageHolder.RootedObject.InstanceId)),
    componentId_(servicePackageHolder.RootedObject.Id.ToString()),
    healthPropertyId_(*ServiceModel::Constants::HealthActivationProperty),
    registrationTimeoutTracker_(),
    runLayout_(servicePackageHolder.RootedObject.HostingHolder.RootedObject.DeploymentFolder),
    serviceTypeInstanceIds_(serviceTypeInstanceIds),
    codePackagesActivationRequested_(false),
    eventWaitAsyncOperation_(),
    deactivateAsyncOperation_(),
    lock_(),
    isOnDemandActivationEnabled_(false),
    activatorCodePackageName_(),
    activatorCodePackageRolloutVersion_(),
    isGuestApplication_(false),
    activatorCodePackageInstanceId_(0)
{
    this->SetTraceId(servicePackageHolder_.Root->TraceId);

    auto registrationTracker = make_shared<ServiceTypeRegistrationTimeoutTracker>(*this);
    registrationTimeoutTracker_ = move(registrationTracker);

    this->InitializeOnDemandActivationInfo();

    auto operationTracker = make_shared<ActivatorCodePackageOperationTracker>(*this);
    operationTracker_ = move(operationTracker);

    WriteNoise(
        TraceType, TraceId,
        "VersionedServicePackage.constructor: {0} ({1}:{2})",
        static_cast<void*>(this),
        servicePackageHolder_.RootedObject.Id,
        servicePackageHolder_.RootedObject.InstanceId);
}

VersionedServicePackage::~VersionedServicePackage()
{
    WriteNoise(
        TraceType, TraceId,
        "VersionedServicePackage.destructor: {0} ({1}:{2})",
        static_cast<void*>(this),
        servicePackageHolder_.RootedObject.Id,
        servicePackageHolder_.RootedObject.InstanceId);
}

AsyncOperationSPtr VersionedServicePackage::BeginOpen(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedServicePackage::BeginSwitch(
    ServicePackageVersionInstance const & toVersion,
    ServicePackageDescription toPackageDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SwitchAsyncOperation>(
        *this,
        toVersion,
        toPackageDescription,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndSwitch(
    AsyncOperationSPtr const & operation)
{
    return SwitchAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedServicePackage::BeginClose(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

void VersionedServicePackage::EndClose(
    AsyncOperationSPtr const & operation)
{
    CloseAsyncOperation::End(operation);
}

void VersionedServicePackage::OnAbort()
{
    CodePackageMap codePackages;
    ServicePackageInstanceEnvironmentContextSPtr packageEnvironment;

    AsyncOperationSPtr waitOperation;
    AsyncOperationSPtr deactivateOperation;
    {
        AcquireWriteLock lock(this->Lock);
        codePackages = move(this->activeCodePackages_);
        packageEnvironment = move(this->environmentContext__);
        waitOperation = move(eventWaitAsyncOperation_);
        deactivateOperation = move(deactivateAsyncOperation_);
    }

    if (waitOperation)
    {
        waitOperation->Cancel();
    }
    if (deactivateOperation)
    {
        deactivateOperation->Cancel();
    }

    // abort all code packages
    for (auto iter = codePackages.begin(); iter != codePackages.end(); ++iter)
    {
        iter->second->AbortAndWaitForTermination();
    }
    codePackages.clear();

    if (packageEnvironment)
    {
        WriteInfo(
            TraceType, TraceId,
            "VersionedServicePackage.OnAbort: Cleaning up Environmnent ServicePackage: {0} {1}", ServicePackageObj.Id, ServicePackageObj.InstanceId);

        this->EnvironmentManagerObj->AbortServicePackageInstanceEnvironment(packageEnvironment, packageDescription__, ServicePackageObj.InstanceId);
    }

    if (this->ServicePackageObj.Id.ApplicationId.IsSystem() &&
        StringUtility::AreEqualCaseInsensitive(ServicePackageObj.Id.ServicePackageName,
            ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
    {
        this->ServicePackageObj.HostingHolder.RootedObject.DnsEnvManager->RemoveDnsFromEnvAndStopMonitorAndWait();
    }

    // unregister the componenet from Healthmanager
    this->HealthManagerObj->UnregisterSource(this->ServicePackageObj.Id, healthPropertyId_);

    // remove any failures registered for this service package
    this->ServiceTypeStateManagerObj->UnregisterFailure(failureId_);

    // Deletes all the ServiceType entries belonging to this ServicePackage from the ServiceTypeStateManager
    this->ServiceTypeStateManagerObj->OnServicePackageDeactivated(serviceTypeInstanceIds_, componentId_);

    // Unregisters service package from LRM.
    this->LocalResourceManagerObj->UnregisterServicePackage(*this);
}

ServicePackage2 const & VersionedServicePackage::get_ServicePackage() const
{
    return this->servicePackageHolder_.RootedObject;
}

HostingSubsystem const & VersionedServicePackage::get_Hosting() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject;
}

EnvironmentManagerUPtr const & VersionedServicePackage::get_EnvironmentManager() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject.ApplicationManagerObj->EnvironmentManagerObj;
}

FabricRuntimeManagerUPtr const & VersionedServicePackage::get_FabricRuntimeManager() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject.FabricRuntimeManagerObj;
}

HostingHealthManagerUPtr const & VersionedServicePackage::get_HealthManagerObj() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject.HealthManagerObj;
}

LocalResourceManagerUPtr const & VersionedServicePackage::get_LocalResourceManager() const
{
    return this->ServicePackageObj.HostingHolder.RootedObject.LocalResourceManagerObj;
}

ServiceTypeStateManagerUPtr const & VersionedServicePackage::get_ServiceTypeStateManager() const
{ 
    return this->FabricRuntimeManagerObj->ServiceTypeStateManagerObj;
}

ServicePackageVersionInstance const VersionedServicePackage::get_CurrentVersionInstance() const
{
    {
        AcquireReadLock lock(Lock);
        return this->currentVersionInstance__;
    }
}

ULONG VersionedServicePackage::GetFailureCount() const
{
    ULONG maxFailureCount = 0;
    ULONG failureCount;
    {
        AcquireReadLock lock(this->Lock);
        for (auto const & kvPair : activeCodePackages_)
        {
            failureCount = kvPair.second->GetMaxContinuousFailureCount();
            if (failureCount > maxFailureCount)
            {
                maxFailureCount = failureCount;
            }
        }
    }

    return maxFailureCount;
}

vector<CodePackageSPtr> VersionedServicePackage::GetCodePackages(wstring const & filterCodePackageName)
{
    vector<CodePackageSPtr> codePackages;
    {
        AcquireReadLock lock(this->Lock);
        for (auto const & kvPair : activeCodePackages_)
        {
            if (kvPair.second->IsImplicitTypeHost &&
                HostingConfig::GetConfig().IncludeGuestServiceTypeHostInCodePackageQuery == false)
            {
                continue;
            }

            if (filterCodePackageName.empty())
            {
                codePackages.push_back(kvPair.second);
            }
            else if (StringUtility::AreEqualCaseInsensitive(kvPair.second->Description.Name, filterCodePackageName))
            {
                codePackages.push_back(kvPair.second);
                break;
            }
        }
    }

    return codePackages;
}

ErrorCode VersionedServicePackage::AnalyzeApplicationUpgrade(
    ServicePackageVersionInstance const & toVersionInstance,
    ServicePackageDescription const & toPackageDescription,
    __out CaseInsensitiveStringSet & affectedRuntimeIds)
{
    auto error = this->TransitionToAnalyzing();
    if (!error.IsSuccess())
    {
        return error;
    }

    ServicePackageVersionInstance currentPackageVersionInstance;
    CodePackageMap currentCodePackages;
    {
        AcquireReadLock lock(this->Lock);
        currentPackageVersionInstance = this->currentVersionInstance__;
        currentCodePackages = this->activeCodePackages_;
    }

    if (currentPackageVersionInstance == toVersionInstance)
    {
        // None of the CodePackages in this ServicePackage will be affected
        this->TransitionToOpened().ReadValue();
        return ErrorCodeValue::Success;
    }

    //it might happen that RG policy on SP level is changing even though the CP itself is not
    //we can optimize this and do a dynamic update of limits on Linux
    //Bug #9358009
    bool hasRgChange = this->PackageDescription.ResourceGovernanceDescription.HasRgChange(
        toPackageDescription.ResourceGovernanceDescription,
        PackageDescription.DigestedCodePackages,
        toPackageDescription.DigestedCodePackages);

    CodePackageSPtr faricTypeHost = nullptr;
    bool hasImpactedCodePackageWithNoRuntimeIds = false;

    for (auto currentCodePackage : currentCodePackages)
    {
        auto const& codePackageInstanceId = currentCodePackage.second->CodePackageInstanceId;

        if (StringUtility::AreEqualCaseInsensitive(codePackageInstanceId.CodePackageName, Constants::ImplicitTypeHostCodePackageName))
        {
            WriteNoise(
                TraceType,
                TraceId,
                "AnalyzeApplicationUpgrade: Code Package found with CodePackageName={0}. No need to analyze with new package description for upgrade.",
                codePackageInstanceId.CodePackageName);
            faricTypeHost = currentCodePackage.second;
            continue;
        }

        bool matchingEntryFound = false;
        for (auto newCodePackage : toPackageDescription.DigestedCodePackages)
        {
            if (StringUtility::AreEqualCaseInsensitive(codePackageInstanceId.CodePackageName, newCodePackage.CodePackage.Name))
            {
                matchingEntryFound = true;
                if (currentCodePackage.second->RolloutVersionValue != newCodePackage.RolloutVersionValue || hasRgChange)
                {
                    // The CodePackage is modified in the new ServicePackage or RG is changing in some way
                    auto affectedRuntimeSize = affectedRuntimeIds.size();
                    error = this->FabricRuntimeManagerObj->GetRuntimeIds(codePackageInstanceId, affectedRuntimeIds);
                    if (affectedRuntimeSize == affectedRuntimeIds.size())
                    {
                        hasImpactedCodePackageWithNoRuntimeIds = true;
                    }
                }

                break;
            }
        }

        if (!matchingEntryFound)
        {
            // The CodePackage is deleted in the new ServicePackage
            error = this->FabricRuntimeManagerObj->GetRuntimeIds(codePackageInstanceId, affectedRuntimeIds);
        }

        if (!error.IsSuccess()) { break; }
    }

    if (error.IsSuccess() && 
        (hasImpactedCodePackageWithNoRuntimeIds || hasRgChange) &&
        faricTypeHost != nullptr)
    {
        //
        // One or more CP has changed with associated RuntimeIds. This can mean either a guest
        // or a watchdog CP version has changed. If the SP has implicit STs (i.e. GuestApp) then
        // we should include impacted runtime Ids from FabricTypeHost to facilitate proper failover
        // for GuestApp.
        //
        // hasRgChange check is here for functional completeness for the case when SP has implicit 
        // STs but there are no guest or watchdog CPs (such SP would probably fail validation 
        // during provisioning).
        //
        error = this->FabricRuntimeManagerObj->GetRuntimeIds(faricTypeHost->CodePackageInstanceId, affectedRuntimeIds);
        if (!error.IsSuccess())
        {
            WriteNoise(
                TraceType,
                TraceId,
                "AnalyzeApplicationUpgrade: GetRuntimeIds failed for CodePackageName={0} with Error {1}.",
                faricTypeHost->CodePackageInstanceId.CodePackageName,
                error);
        }
    }

    this->TransitionToOpened().ReadValue();
    return error;
}

ErrorCode VersionedServicePackage::WriteCurrentPackageFile(
    ServicePackageVersion const & packageVersion)
{
    wstring currentPackageFilePath = runLayout_.GetCurrentServicePackageFile(
        this->ServicePackageObj.Id.ApplicationId.ToString(),
        this->ServicePackageObj.ServicePackageName,
        this->ServicePackageObj.Id.PublicActivationId);

    wstring packageFilePath = runLayout_.GetServicePackageFile(
        this->ServicePackageObj.Id.ApplicationId.ToString(),
        this->ServicePackageObj.ServicePackageName,
        packageVersion.RolloutVersionValue.ToString());

    return File::Copy(packageFilePath, currentPackageFilePath, true);
}

ErrorCode VersionedServicePackage::LoadCodePackages(
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    ServicePackageDescription const & packageDescription,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext,
    bool onlyActivatorCodePackage,
    _Out_ CodePackageMap & codePackages)
{
    auto removeServiceFabricRuntimeAccess = packageDescription.SFRuntimeAccessDescription.RemoveServiceFabricRuntimeAccess;
    auto useSFReplicatedStore = packageDescription.SFRuntimeAccessDescription.UseServiceFabricReplicatedStore;

    // Disable SFReplicated Store scenarios (e.g. SFVolumeDisk) if preview features are not enabled on the cluster
    if (useSFReplicatedStore)
    {
        if ((!CommonConfig::GetConfig().EnableUnsupportedPreviewFeatures) || (!HostingConfig::GetConfig().IsSFVolumeDiskServiceEnabled))
        {
            useSFReplicatedStore = false;
            WriteInfo(
                TraceType,
                TraceId,
                "LoadCodePackages: SFVolumeDisk is disabled since feature is not enabled on the cluster.");
        }
    }

    for (auto & dcpDesc : packageDescription.DigestedCodePackages)
    {
        if (onlyActivatorCodePackage && !dcpDesc.CodePackage.IsActivator)
        {
            continue;
        }

        int64 activatorInstanceId;
        auto error = this->GetActivatorCodePackageInstanceId(dcpDesc.CodePackage.Name, activatorInstanceId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                TraceId,
                "LoadCodePackages: GetActivatorCodePackageInstanceId failed with Error={0}. CPName={1}.",
                error,
                dcpDesc.CodePackage.Name);

            return error;
        }

        wstring userId;
        this->GetUserId(dcpDesc.CodePackage.Name, dcpDesc.RunAsPolicy, false, userId);

        wstring setuppointRunas;
        this->GetUserId(dcpDesc.CodePackage.Name, dcpDesc.SetupRunAsPolicy, true, setuppointRunas);

        ProcessDebugParameters debugParameters;
        this->GetProcessDebugParams(dcpDesc, debugParameters);

        auto codePackage = make_shared<CodePackage>(
            this->ServicePackageObj.HostingHolder,
            VersionedServicePackageHolder(*this, this->CreateComponentRoot()),
            dcpDesc,
            this->ServicePackageObj.Id,
            this->ServicePackageObj.InstanceId,
            servicePackageVersionInstance,
            this->ServicePackageObj.ApplicationName,
            dcpDesc.RolloutVersionValue,
            envContext,
            activatorInstanceId,
            move(EnvironmentMap()),
            userId,
            setuppointRunas,
            dcpDesc.IsShared,
            debugParameters,
            GetCodePackagePortMappings(dcpDesc.ContainerPolicies, envContext),
            removeServiceFabricRuntimeAccess);

        codePackages.insert(
            make_pair(
                codePackage->Description.Name,
                move(codePackage)));

        if (onlyActivatorCodePackage)
        {
            // There can only be one activator CP in a SP.
            break;
        }
    }

    if (onlyActivatorCodePackage && codePackages.size() > 0)
    {
        // There can only be one activator CP in a SP.
        return ErrorCode(ErrorCodeValue::Success);
    }

    // check for guest service types
    vector<GuestServiceTypeInfo> guestServiceTypes;

    for (auto const & serviceType : packageDescription.DigestedServiceTypes.ServiceTypes)
    {
        if (serviceType.UseImplicitHost)
        {
            GuestServiceTypeInfo typeInfo(serviceType.ServiceTypeName, serviceType.IsStateful);
            guestServiceTypes.push_back(move(typeInfo));
        }
    }

    if (guestServiceTypes.size() > 0)
    {
        CodePackageSPtr typeHostCodePackage;
        auto error = CreateImplicitTypeHostCodePackage(
            packageDescription.ManifestVersion,
            servicePackageVersionInstance,
            guestServiceTypes,
            envContext,
            useSFReplicatedStore,
            typeHostCodePackage);

        if (!error.IsSuccess())
        {
            return error;
        }

        codePackages.insert(
            make_pair(
                typeHostCodePackage->Description.Name,
                move(typeHostCodePackage)));
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode VersionedServicePackage::CreateImplicitTypeHostCodePackage(
    wstring const & codePackageVersion,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    vector<GuestServiceTypeInfo> const & serviceTypes,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext,
    bool useSFReplicatedStore,
    __out CodePackageSPtr & typeHostCodePackage)
{
    wstring typeHostPath;

    //no need to set this if we do not use FabricTypeHost.exe
    if (!HostingConfig::GetConfig().HostGuestServiceTypeInProc ||
        useSFReplicatedStore)
    {
        auto error = this->ServicePackageObj.HostingHolder.RootedObject.GetTypeHostPath(typeHostPath, useSFReplicatedStore);
        if (!error.IsSuccess()) { return error; }
    }

    wstring arguments = L"";
    for (auto serviceType : serviceTypes)
    {
        arguments = wformatString("{0} -type:{1}", arguments, serviceType.ServiceTypeName);
    }

    DigestedCodePackageDescription codePackageDescription;
    if (useSFReplicatedStore)
    {
        codePackageDescription.CodePackage.Name = (wstring)Constants::BlockStoreServiceCodePackageName;
    }
    else
    {
        codePackageDescription.CodePackage.Name = (wstring)Constants::ImplicitTypeHostCodePackageName;
    }

    int64 activatorInstanceId;
    auto error = this->GetActivatorCodePackageInstanceId(codePackageDescription.CodePackage.Name, activatorInstanceId);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CreateImplicitTypeHostCodePackage: GetActivatorCodePackageInstanceId failed with Error={0}. CPName={1}.",
            error,
            codePackageDescription.CodePackage.Name);

        return error;
    }

    codePackageDescription.CodePackage.Version = codePackageVersion;
    codePackageDescription.CodePackage.IsShared = false;
    codePackageDescription.CodePackage.IsActivator = isOnDemandActivationEnabled_;
    codePackageDescription.CodePackage.EntryPoint.EntryPointType = EntryPointType::Exe;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.Program = typeHostPath;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.Arguments = arguments;
    codePackageDescription.CodePackage.EntryPoint.ExeEntryPoint.WorkingFolder = ServiceModel::ExeHostWorkingFolder::Work;

    RolloutVersion rolloutVersion = RolloutVersion::Invalid;

    typeHostCodePackage = make_shared<CodePackage>(
        this->ServicePackageObj.HostingHolder,
        VersionedServicePackageHolder(*this, this->CreateComponentRoot()),
        codePackageDescription,
        this->ServicePackageObj.Id,
        this->ServicePackageObj.InstanceId,
        servicePackageVersionInstance,
        this->ServicePackageObj.ApplicationName,
        rolloutVersion,
        envContext,
        activatorInstanceId,
        move(EnvironmentMap()),
        L"" /* runAsId */,
        L"" /* setupRunas */,
        false /* isShared */,
        ProcessDebugParameters(),
        map<wstring, wstring>(),
        false /* removeServiceFabricRuntimeAccess */,
        serviceTypes);

    return ErrorCode(ErrorCodeValue::Success);
}

void VersionedServicePackage::ActivateCodePackagesAsync(
    CodePackageMap const & codePackages,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(ActivateCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
   
    auto operation = AsyncOperation::CreateAndStart<ActivateCodePackagesAsyncOperation>(
            *this,
            codePackages,
            timeout,
            [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishActivateCodePackages(operation, false, asyncCallback); },
            parent);
        this->FinishActivateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishActivateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    ErrorCode error;
   
    error = ActivateCodePackagesAsyncOperation::End(operation, failed, completed);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            TraceId,
            "End(ActivateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
            ServicePackageObj.Id,
            ServicePackageObj.InstanceId,
            completed.size(),
            failed.size(),
            error);
    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::UpdateCodePackagesAsync(
    CodePackageMap const & codePackages,
    ServicePackageDescription const & newPackageDescription,
    ServicePackageVersionInstance const & servicePackageVersionInstance,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(UpdateCodePackages): Id={0}:{1}, CodePackageCount={2}, ServicePackageVersionInstance={3}, Timeout={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        servicePackageVersionInstance,
        timeout);

    auto operation = AsyncOperation::CreateAndStart<UpdateCodePackagesAsyncOperation>(
        *this,
        codePackages,
        newPackageDescription,
        servicePackageVersionInstance,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishUpdateCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishUpdateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishUpdateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = UpdateCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(UpdateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::DeactivateCodePackagesAsync(
    CodePackageMap const & codePackages,
    TimeSpan const timeout,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(DeactivateCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
    auto operation = AsyncOperation::CreateAndStart<DeactivateCodePackagesAsyncOperation>(
        *this,
        codePackages,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishDeactivateCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishDeactivateCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishDeactivateCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = DeactivateCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(DectivateCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

void VersionedServicePackage::AbortCodePackagesAsync(
    CodePackageMap const & codePackages,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
    AsyncOperationSPtr const & parent)
{
    TimeSpan timeout = TimeSpan::MaxValue;
    WriteNoise(
        TraceType,
        TraceId,
        "Begin(AbortCodePackages): Id={0}:{1}, CodePackageCount={2}, Timeout={3}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        codePackages.size(),
        timeout);
    auto operation = AsyncOperation::CreateAndStart<AbortCodePackagesAsyncOperation>(
        *this,
        codePackages,
        timeout,
        [this, asyncCallback](AsyncOperationSPtr const & operation) { this->FinishAbortCodePackages(operation, false, asyncCallback); },
        parent);
    this->FinishAbortCodePackages(operation, true, asyncCallback);
}

void VersionedServicePackage::FinishAbortCodePackages(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously,
    CodePackagesAsyncOperationCompletedCallback const & asyncCallback)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    CodePackageMap failed;
    CodePackageMap completed;
    auto error = AbortCodePackagesAsyncOperation::End(operation, failed, completed);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "End(AbortCodePackages): Id={0}:{1}, CompletedCount={2}, FailedCount={3}, ErrorCode={4}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        completed.size(),
        failed.size(),
        error);

    asyncCallback(operation, error, completed, failed);
}

AsyncOperationSPtr VersionedServicePackage::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & request,
    int64 requestorInstanceId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackagesAsyncOperation>(
        *this,
        request,
        requestorInstanceId,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackagesAsyncOperation::End(operation);
}

AsyncOperationSPtr VersionedServicePackage::BeginProcessActivatorCodePackageTerminated(
    int64 activatorCodePackageInstanceId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivatorCodePackageTerminatedAsyncOperation>(
        *this,
        activatorCodePackageInstanceId,
        callback,
        parent);
}

void VersionedServicePackage::EndProcessActivatorCodePackageTerminated(
    AsyncOperationSPtr const & operation)
{
    ActivatorCodePackageTerminatedAsyncOperation::End(operation).ReadValue();
}

void VersionedServicePackage::ProcessDependentCodePackageEvent(
    CodePackageEventDescription && eventDesc,
    int64 targetActivatorCodePackageInstanceId)
{
    CodePackageSPtr activatorCodePackage;

    {
        AcquireReadLock readLock(this->Lock);

        auto currentState = this->GetState_CallerHoldsLock();
        if (currentState == VersionedServicePackage::Aborted ||
            currentState >= VersionedServicePackage::Closing)
        {
            WriteNoise(
                TraceType,
                TraceId,
                "ProcessDependentCodePackageEvent: Ignoring Event={0} as current state is '{1}'.",
                eventDesc,
                this->StateToString(currentState));

            return;
        }

        if (activatorCodePackageInstanceId_ == 0 || 
            activatorCodePackageInstanceId_ != targetActivatorCodePackageInstanceId)
        {
            WriteInfo(
                TraceType,
                TraceId,
                "ProcessDependentCodePackageEvent: Ignoring Event={0} due to ActivatorCodePackageInstanceId mismatch (Received={1}, Current={2}). ServicePackageInstanceId={3}.",
                targetActivatorCodePackageInstanceId,
                activatorCodePackageInstanceId_,
                this->ServicePackageObj.Id);
            return;
        }

        auto iter = activeCodePackages_.find(activatorCodePackageName_);
        
        ASSERT_IF(
            iter == activeCodePackages_.end(),
            "ProcessDependentCodePackageEvent: ActivatorCodePackage (InstanceId={0}) must be present. ServicePackageInstanceId={1}",
            activatorCodePackageInstanceId_,
            this->ServicePackageObj.Id);

        activatorCodePackage = iter->second;
    }

    //
    // Dispatch notification on a diferent thread.
    //
    Threadpool::Post(
        [this, activatorCodePackage, eventDesc, targetActivatorCodePackageInstanceId]()
        { 
            activatorCodePackage->SendDependentCodePackageEvent(
                eventDesc, 
                targetActivatorCodePackageInstanceId);
        });
}

ErrorCode VersionedServicePackage::GetActivatorCodePackage(CodePackageSPtr & activatorCodePackage)
{
    AcquireReadLock readLock(this->Lock);

    auto iter = activeCodePackages_.find(activatorCodePackageName_);
    if (iter != activeCodePackages_.end())
    {
        activatorCodePackage = iter->second;
        return ErrorCode::Success();
    }

    return ErrorCode(ErrorCodeValue::CodePackageNotFound);
}

void VersionedServicePackage::OnServiceTypesUnregistered(
    vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    registrationTimeoutTracker_->OnServiceTypesUnregistered(serviceTypeInstanceIds);
}

void VersionedServicePackage::OnServiceTypeRegistrationNotFound(
    uint64 const registrationTableVersion,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId)
{
    auto failureCount = this->GetFailureCount();

    WriteNoise(
        TraceType,
        TraceId,
        "OnServiceTypeRegistrationNotFound: {0}:{1}:{2}, VersionedServiceTypeId={3}, RegistrationTableVersion={4}, FailureCount={5}",
        ServicePackageObj.Id,
        ServicePackageObj.InstanceId,
        CurrentVersionInstance,
        versionedServiceTypeId,
        registrationTableVersion,
        failureCount);

    if (failureCount > (ULONG)HostingConfig::GetConfig().ServiceTypeDisableFailureThreshold)
    {
        bool disable = false;
        {
            AcquireReadLock lock(this->Lock);
            if (this->GetState_CallerHoldsLock() == VersionedServicePackage::Opened)
            {
                this->ServiceTypeStateManagerObj->RegisterFailure(failureId_);
                disable = true;
            }
        }

        if (disable)
        {
            this->ServiceTypeStateManagerObj->Disable(
                ServiceTypeInstanceIdentifier(
                    versionedServiceTypeId.Id, 
                    servicePackageInstanceId.ActivationContext,
                    servicePackageInstanceId.PublicActivationId),
                this->ServicePackageObj.ApplicationName,
                failureId_);
        }
    }

    if (registrationTimeoutTracker_->HasRegistrationTimedout(versionedServiceTypeId.Id.ServiceTypeName))
    {
        this->ServiceTypeStateManagerObj->OnServiceTypeRegistrationNotFoundAfterTimeout(
            ServiceTypeInstanceIdentifier(
                versionedServiceTypeId.Id, 
                servicePackageInstanceId.ActivationContext,
                servicePackageInstanceId.PublicActivationId),
            registrationTableVersion);
    }
}

map<wstring, wstring> VersionedServicePackage::GetCodePackagePortMappings(
    ContainerPoliciesDescription const & description,
    ServicePackageInstanceEnvironmentContextSPtr const & envContext)
{
    map<wstring, wstring> portBindings;
    for (auto iter = description.PortBindings.begin(); iter != description.PortBindings.end(); iter++)
    {
        auto internalPort = iter->ContainerPort;
        for (auto it = envContext->Endpoints.begin(); it != envContext->Endpoints.end(); it++)
        {
            if (StringUtility::AreEqualCaseInsensitive(iter->EndpointResourceRef, (*it)->Name))
            {
                //If containerport was specified as ), use the endpoint port as exposed port.
                if (internalPort == 0)
                {
                    internalPort = (*it)->Port;
                }
                auto containerProtocolType = ((*it)->Protocol == ProtocolType::Udp) ? ProtocolType::Udp : ProtocolType::Tcp;

                auto containerPort = wformatString("{0}/{1}", StringUtility::ToWString(internalPort), ProtocolType::EnumToString(containerProtocolType));
                portBindings.insert(make_pair(containerPort, StringUtility::ToWString((*it)->Port)));
                break;
            }
        }
    }
    return portBindings;
}

ErrorCode VersionedServicePackage::IsCodePackageLockFilePresent(
    _Out_ bool & isCodePackageLockFilePresent)
{
    auto codePackages = this->GetCodePackages();
    for (auto iter = codePackages.begin(); iter != codePackages.end(); ++iter)
    {
        isCodePackageLockFilePresent = !(*iter)->DebugParameters.LockFile.empty() && File::Exists((*iter)->DebugParameters.LockFile);
        if (isCodePackageLockFilePresent)
        {
            WriteInfo(
                TraceType,
                TraceId,
                "IsCodePackageLockFilePresent: CodePackage={0} has lock file present at {1}",
                (*iter)->CodePackageInstanceId,
                (*iter)->DebugParameters.LockFile);
            break;
        }
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr VersionedServicePackage::BeginTerminateFabricTypeHostCodePackage(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    int64 instanceId)
{
    return AsyncOperation::CreateAndStart<TerminateFabricTypeHostAsyncOperation>(
        *this,
        instanceId,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndTerminateFabricTypeHostCodePackage(
    AsyncOperationSPtr const & operation,
    __out int64 & instanceId)
{
    return TerminateFabricTypeHostAsyncOperation::End(operation, instanceId);
}

AsyncOperationSPtr VersionedServicePackage::BeginRestartCodePackageInstance(
    wstring const & codePackageName,
    int64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RestartCodePackageInstanceAsyncOperation>(
        *this,
        codePackageName,
        instanceId,
        timeout,
        callback,
        parent);
}

ErrorCode VersionedServicePackage::EndRestartCodePackageInstance(AsyncOperationSPtr const & operation)
{
    return RestartCodePackageInstanceAsyncOperation::End(operation);
}

void VersionedServicePackage::OnDockerHealthCheckUnhealthy(wstring const & codePackageName, int64 instanceId)
{
    auto operation = this->BeginRestartCodePackageInstance(
        codePackageName,
        instanceId,
        HostingConfig::GetConfig().ContainerTerminationTimeout,
        [this](AsyncOperationSPtr const & operation) { this->EndRestartCodePackageInstance(operation); },
        this->CreateAsyncOperationRoot());
}

ErrorCode VersionedServicePackage::TryGetPackageSecurityPrincipalInfo(
    __in std::wstring const & name,
    __out SecurityPrincipalInformationSPtr & info)
{
    if (this->ServicePackageObj.ApplicationEnvironment->PrincipalsContext != nullptr)
    {
        DigestedCodePackageDescription package;
        auto packages = this->PackageDescription.DigestedCodePackages;
        for (auto iter = packages.begin(); iter != packages.end(); ++iter)
        {
            if (StringUtility::AreEqualCaseInsensitive(iter->Name, name))
            {
                return this->ServicePackageObj.ApplicationEnvironment->PrincipalsContext->TryGetPrincipalInfo(iter->RunAsPolicy.UserRef, info);
            }
        }
    }

    return ErrorCodeValue::NotFound;
}

void VersionedServicePackage::InitializeOnDemandActivationInfo()
{
    auto activationContext = servicePackageHolder_.RootedObject.Id.ActivationContext;

    auto implicitTypeCount = 0;
    auto implicitStatefulTypeCount = 0;
    auto normalTypeCount = 0;

    for (auto serviceType : packageDescription__.DigestedServiceTypes.ServiceTypes)
    {
        if (serviceType.UseImplicitHost)
        {
            ++implicitTypeCount;

            if (serviceType.IsStateful)
            {
                ++implicitStatefulTypeCount;
            }
        }
        else
        {
            ++normalTypeCount;
        }
    }

    auto hasActivatorCp = false;
    DigestedCodePackageDescription activatorCpDesc;

    for (auto const & digectedCp : packageDescription__.DigestedCodePackages)
    {
        if (digectedCp.CodePackage.IsActivator)
        {
            hasActivatorCp = true;
            activatorCodePackageName_ = digectedCp.Name;
            activatorCodePackageRolloutVersion_ = digectedCp.RolloutVersionValue;

            break;
        }
    }

    isGuestApplication_ = (implicitTypeCount > 0);

    ASSERT_IF(
        isGuestApplication_ && hasActivatorCp,
        "A guest app cannot have explicit activator CP. ServicePackageInstancdId={0} PackageDescription:{1} ",
        servicePackageHolder_.RootedObject.Id,
        packageDescription__);

    auto isEligibleGuestApp = 
        isGuestApplication_ &&
        normalTypeCount == 0 &&
        activationContext.IsExclusive &&
        HostingConfig::GetConfig().HostGuestServiceTypeInProc;

    if (isEligibleGuestApp && 
        implicitStatefulTypeCount == 0 &&
        HostingConfig::GetConfig().DisableOnDemandActivationForStatelessGuestApp)
    {
        WriteInfo(
            TraceType,
            TraceId,
            "Disabling OnDemandActivation as DisableOnDemandActivationForStatelessGuestApp=true. ServicePackageInstanceId:{0}",
            servicePackageHolder_.RootedObject.Id);
        return;
    }

    if (isEligibleGuestApp || hasActivatorCp)
    {
        isOnDemandActivationEnabled_ = true;

        WriteInfo(
            TraceType,
            TraceId,
            "OnDemandActivationEnabled: {0} PackageDescription={1}",
            isOnDemandActivationEnabled_,
            packageDescription__);

        if (!hasActivatorCp)
        {
            activatorCodePackageName_ = move(this->GetImplicitTypeHostName());
            activatorCodePackageRolloutVersion_ = RolloutVersion::Invalid;
        }
    }
}

ErrorCode VersionedServicePackage::GetActivatorCodePackageInstanceId(
    wstring const & codePackageName,
    _Out_ int64 & instanceId)
{
    if (!isOnDemandActivationEnabled_ || codePackageName == activatorCodePackageName_)
    {
        instanceId = 0;
        return ErrorCode::Success();
    }

    instanceId = this->GetActivatorCodePackageInstanceId();

    if (instanceId == 0)
    {
        // This means that activator CP has terminated and
        // currently there is no running activator CP.
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    return ErrorCode::Success();
}

int64 VersionedServicePackage::GetActivatorCodePackageInstanceId()
{
    AcquireReadLock readerLock(this->Lock);
    return activatorCodePackageInstanceId_;
}

wstring VersionedServicePackage::GetImplicitTypeHostName()
{
    auto useSfReplicatedStore = false;

    {
        AcquireReadLock(this->Lock);
        useSfReplicatedStore = packageDescription__.SFRuntimeAccessDescription.UseServiceFabricReplicatedStore;
    }

    auto typeHostName = useSfReplicatedStore ?
        *Constants::BlockStoreServiceCodePackageName :
        *Constants::ImplicitTypeHostCodePackageName;

    return move(typeHostName);
}

void VersionedServicePackage::GetUserId(
    wstring const & codePackageName,
    RunAsPolicyDescription const & runAsPolicyDesc,
    bool isSetupRunAsPolicy,
    _Out_ wstring & userId)
{
    if (!runAsPolicyDesc.CodePackageRef.empty())
    {
        ASSERT_IFNOT(
            StringUtility::AreEqualCaseInsensitive(codePackageName, runAsPolicyDesc.CodePackageRef),
            "Error in the DigestedCodePackage element. The CodePackageRef '{0}' in RunAsPolicy does not match the CodePackage Name '{1}'.",
            runAsPolicyDesc.CodePackageRef,
            codePackageName);

        auto error = this->ServicePackageObj.ApplicationEnvironment->TryGetUserId(runAsPolicyDesc.UserRef, userId);
        if (!error.IsSuccess())
        {
            Assert::CodingError(
                "UserId {0} must be present in the application environment context.",
                runAsPolicyDesc.UserRef);
        }

        return;
    }

    if(!isSetupRunAsPolicy)
    {
        // If the ServicePackage being activated is ResourceMonitor or BackupRestore service, 
        // then we add a special id so that we run it as part of windows administrator group
        auto svcPkgId = this->ServicePackageObj.Id;
        
        if (svcPkgId.ApplicationId.IsSystem() &&
            (svcPkgId.ServicePackageName == L"RMS" || 
             svcPkgId.ServicePackageName == L"BRS"))
        {
            userId = *Constants::WindowsFabricAdministratorsGroupAllowedUser;
        }
    }
}

void VersionedServicePackage::GetCurrentVersionInstanceAndPackageDescription(
    _Out_ ServicePackageVersionInstance & versionInstance,
    _Out_ ServicePackageDescription & packageDescription)
{
    AcquireReadLock lock(this->Lock);

    versionInstance = currentVersionInstance__;
    packageDescription = packageDescription__;
}

void VersionedServicePackage::OnActivatorCodePackageInitialized(
    int64 activatorCodePackageInstanceId)
{
    AcquireWriteLock writeLock(this->Lock);
    activatorCodePackageInstanceId_ = activatorCodePackageInstanceId;
}

void VersionedServicePackage::GetProcessDebugParams(
    DigestedCodePackageDescription const & dcpDesc,
    _Out_ ProcessDebugParameters & debugParams)
{
    EnvironmentMap envMap;
    auto envBlock = dcpDesc.DebugParameters.EnvironmentBlock;
    LPVOID penvBlock = &envBlock;
    Environment::FromEnvironmentBlock(penvBlock, envMap);

    auto codePackagePath = this->ServicePackageObj.runLayout_.GetCodePackageFolder(
        this->ServicePackageObj.ApplicationEnvironment->ApplicationId.ToString(),
        this->ServicePackageObj.ServicePackageName,
        dcpDesc.CodePackage.Name,
        dcpDesc.CodePackage.Version,
        false);

    auto lockFilePath = dcpDesc.DebugParameters.LockFile;
    if (!lockFilePath.empty() && !Path::IsPathRooted(lockFilePath))
    {
        lockFilePath = Path::Combine(codePackagePath, lockFilePath);
    }

    auto debugParamsFile = dcpDesc.DebugParameters.DebugParametersFile;
    if (!debugParamsFile.empty() && !Path::IsPathRooted(debugParamsFile))
    {
        debugParamsFile = Path::Combine(codePackagePath, debugParamsFile);
    }

    debugParams =ProcessDebugParameters(
        dcpDesc.DebugParameters.ExePath,
        dcpDesc.DebugParameters.Arguments,
        lockFilePath,
        dcpDesc.DebugParameters.WorkingFolder,
        debugParamsFile,
        envMap,
        dcpDesc.DebugParameters.ContainerEntryPoints,
        dcpDesc.DebugParameters.ContainerMountedVolumes,
        dcpDesc.DebugParameters.ContainerEnvironmentBlock,
        dcpDesc.DebugParameters.ContainerLabels);
}

