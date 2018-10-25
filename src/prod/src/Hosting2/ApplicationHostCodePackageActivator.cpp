// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("ApplicationHostCodePackageActivator");

struct ApplicationHostCodePackageActivator::EventHandlerInfo
{
    EventHandlerInfo(
        ULONGLONG hanlderId,
        ComPointer<IFabricCodePackageEventHandler> const & eventHandler,
        wstring const & sourceComObjectId)
        : HanlderId(hanlderId)
        , EventHandler(eventHandler)
        , SourceComObjectId(sourceComObjectId)
    {
    }

    ULONGLONG HanlderId;
    wstring SourceComObjectId;
    ComPointer<IFabricCodePackageEventHandler> EventHandler;
};

class ApplicationHostCodePackageActivator::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        _In_ ApplicationHostCodePackageActivator & owner,
        vector<wstring> const & codePackageNames,
        EnvironmentMap const & environment,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageNames_(codePackageNames)
        , environment_(environment)
        , timeoutHelper_(timeout)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->ActivateCodePackage(thisSPtr);
    }

    void ActivateCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = this->CreateActivateCodePackageRequestMessage();

        auto operation = owner_.BeginSendRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateCodePackage(operation, false);
            },
            thisSPtr);

        this->FinishActivateCodePackage(operation, true);
    }

    void FinishActivateCodePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ApplicationHostCodePackageOperationReply replyBody;
        auto error = owner_.EndSendRequest(operation, replyBody);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishActivateCodePackage: owner_.EndSendMessage() : Error={0}.",
                error);

            this->TryComplete(operation->Parent, error);
            return;
        }

        if (!replyBody.Error.IsSuccess())
        {
            owner_.TraceWarning(
                move(L"ActivateAsyncOperation"),
                move(codePackageNames_),
                replyBody.Error);
        }

        this->TryComplete(operation->Parent, replyBody.Error);
    }

    ApplicationHostCodePackageOperationRequest CreateActivateCodePackageRequestMessage()
    {
        auto operationType =
            codePackageNames_.empty() ?
            ApplicationHostCodePackageOperationType::ActivateAll :
            ApplicationHostCodePackageOperationType::Activate;

        ApplicationHostCodePackageOperationRequest requestBody(
            operationType,
            owner_.GetHostContext(),
            owner_.GetCodePackageContext(),
            codePackageNames_,
            environment_,
            timeoutHelper_.GetRemainingTime().Ticks);
        
        return move(requestBody);
    }

protected:
    ApplicationHostCodePackageActivator & owner_;
    vector<wstring> codePackageNames_;
    EnvironmentMap environment_;
    TimeoutHelper timeoutHelper_;
};

class ApplicationHostCodePackageActivator::DeactivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        _In_ ApplicationHostCodePackageActivator & owner,
        vector<wstring> const & codePackageNames,
        bool isGraceful,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , codePackageNames_(codePackageNames)
        , isGraceful_(isGraceful)
        , timeoutHelper_(timeout)
    {
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->DeactivateCodePackage(thisSPtr);
    }

    void DeactivateCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto request = this->CreateDeactivateCodePackageRequestMessage();

        auto operation = owner_.BeginSendRequest(
            move(request),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeactivateCodePackage(operation, false);
            },
            thisSPtr);

        this->FinishDeactivateCodePackage(operation, true);
    }

    void FinishDeactivateCodePackage(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ApplicationHostCodePackageOperationReply replyBody;
        auto error = owner_.EndSendRequest(operation, replyBody);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "FinishDeactivateCodePackage: owner_.EndSendMessage() : Error={0}.",
                error);

            this->TryComplete(operation->Parent, error);
            return;
        }

        if (!replyBody.Error.IsSuccess())
        {
            owner_.TraceWarning(
                move(wformatString("DeactivateAsyncOperation(isGraceful={0})", isGraceful_)),
                move(codePackageNames_),
                replyBody.Error);
        }

        this->TryComplete(operation->Parent, replyBody.Error);
    }

    ApplicationHostCodePackageOperationRequest CreateDeactivateCodePackageRequestMessage()
    {
        ApplicationHostCodePackageOperationType::Enum operationType;

        if (codePackageNames_.empty())
        {
            operationType =
                isGraceful_ ?
                ApplicationHostCodePackageOperationType::DeactivateAll :
                ApplicationHostCodePackageOperationType::AbortAll;
        }
        else
        {
            operationType =
                isGraceful_ ?
                ApplicationHostCodePackageOperationType::Deactivate :
                ApplicationHostCodePackageOperationType::Abort;
        }

        ApplicationHostCodePackageOperationRequest requestBody(
            operationType,
            owner_.GetHostContext(),
            owner_.GetCodePackageContext(),
            codePackageNames_,
            EnvironmentMap(),
            timeoutHelper_.GetRemainingTime().Ticks);

        return move(requestBody);
    }

protected:
    ApplicationHostCodePackageActivator & owner_;
    vector<wstring> codePackageNames_;
    bool isGraceful_;
    TimeoutHelper timeoutHelper_;
};

ApplicationHostCodePackageActivator::ApplicationHostCodePackageActivator(
    ComponentRoot const & root)
    : RootedObject(root)
    , eventHandlersLock_()
    , eventHandlers_()
    , handlerIdTracker_(DateTime::Now().Ticks)
    , comSourcePointers_()
{
}

ApplicationHostCodePackageActivator::~ApplicationHostCodePackageActivator()
{
}

AsyncOperationSPtr ApplicationHostCodePackageActivator::BeginActivateCodePackage(
    vector<wstring> const & codePackageNames,
    EnvironmentMap && environment,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        codePackageNames,
        environment,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostCodePackageActivator::EndActivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    return ActivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHostCodePackageActivator::BeginDeactivateCodePackage(
    vector<wstring> const & codePackageNames,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        codePackageNames,
        true /* isGraceful */,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostCodePackageActivator::EndDeactivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

void ApplicationHostCodePackageActivator::AbortCodePackage(
    vector<wstring> const & codePackageNames)
{
    auto timeout = HostingConfig::GetConfig().ActivationTimeout;

    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        codePackageNames,
        false /* isGraceful */,
        timeout,
        [waiter](AsyncOperationSPtr const &operation)
    {
        auto error = DeactivateAsyncOperation::End(operation);
        waiter->SetError(error);
        waiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (!waiter->WaitOne(timeout))
    {
        auto cpNames = codePackageNames;

        this->TraceWarning(
            move(L"AbortCodePackage"),
            move(cpNames),
            ErrorCode(ErrorCodeValue::Timeout));
    }
}

ULONGLONG ApplicationHostCodePackageActivator::RegisterCodePackageEventHandler(
    ComPointer<IFabricCodePackageEventHandler> const & eventHandlerCPtr,
    wstring const & sourceId)
{
    ULONGLONG handleId;

    {
        AcquireWriteLock lock(eventHandlersLock_);

        handleId = ++handlerIdTracker_;

        eventHandlers_.push_back(make_shared<EventHandlerInfo>(
            handleId,
            eventHandlerCPtr,
            sourceId));
    }

    return handleId;
}

void ApplicationHostCodePackageActivator::UnregisterCodePackageEventHandler(ULONGLONG handlerId)
{
    AcquireWriteLock lock(eventHandlersLock_);

    eventHandlers_.erase(remove_if(eventHandlers_.begin(), eventHandlers_.end(),
        [handlerId](shared_ptr<EventHandlerInfo> const & eventHandler)
        {
            return (eventHandler->HanlderId == handlerId);
        }),
        eventHandlers_.end());
}

void ApplicationHostCodePackageActivator::RegisterComCodePackageActivator(
    wstring const& sourceId,
    ITentativeApplicationHostCodePackageActivator const & comActivator)
{
    AcquireWriteLock lock(eventHandlersLock_);

    auto iter = comSourcePointers_.find(sourceId);
    ASSERT_IFNOT(iter == comSourcePointers_.end(), "ComSource {0} already exists", sourceId);

    auto sourcePointer = ReferencePointer<ITentativeApplicationHostCodePackageActivator>(
        const_cast<ITentativeApplicationHostCodePackageActivator*>(&comActivator));
    comSourcePointers_.insert(make_pair(sourceId, sourcePointer));
}

void ApplicationHostCodePackageActivator::UnregisterComCodePackageActivator(
    wstring const& sourceId)
{
    AcquireWriteLock lock(eventHandlersLock_);
    
    auto iter = comSourcePointers_.find(sourceId);
    ASSERT_IF(iter == comSourcePointers_.end(), "ComSource {0} does not exist", sourceId);
    
    comSourcePointers_.erase(iter);
}

void ApplicationHostCodePackageActivator::CopyEventHandlers(
    vector<shared_ptr<EventHandlerInfo>> & eventHandlersSnap)
{
    AcquireReadLock lock(eventHandlersLock_);
    eventHandlersSnap = eventHandlers_;
}

void ApplicationHostCodePackageActivator::NotifyEventHandlers(
    CodePackageEventDescription const & eventDesc)
{
    vector<shared_ptr<EventHandlerInfo>> eventHandlersSnap;
    this->CopyEventHandlers(eventHandlersSnap);

    if (eventHandlersSnap.size() == 0)
    {
        return;
    }

    ScopedHeap heap;
    FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION fabricEventDesc = {};

    auto error = eventDesc.ToPublicApi(heap, fabricEventDesc);
    
    ASSERT_IFNOT(
        error.IsSuccess(),
        "CodePackageEventDescription::ToPublicApi() failed witrh Error={0}",
        error);

    for (auto eventHandlerInfo : eventHandlersSnap)
    {
        auto const & eventHandler = eventHandlerInfo->EventHandler;
        auto const & sourceId = eventHandlerInfo->SourceComObjectId;

        ComPointer<ITentativeApplicationHostCodePackageActivator> comSource;
        if (this->TryGetComSource(sourceId, comSource))
        {
            eventHandler->OnCodePackageEvent(
                comSource.GetRawPointer(),
                &fabricEventDesc);
        }
    }
}

bool ApplicationHostCodePackageActivator::TryGetComSource(
    wstring const & sourceId,
    _Out_ ComPointer<ITentativeApplicationHostCodePackageActivator> & comSource)
{
    ReferencePointer<ITentativeApplicationHostCodePackageActivator> snap;
    {
        AcquireReadLock lock(eventHandlersLock_);
        auto sourceIter = comSourcePointers_.find(sourceId);
        if (sourceIter != comSourcePointers_.end())
        {
            snap = sourceIter->second;
            if (snap.GetRawPointer() != NULL && snap->TryAddRef() > 0u)
            {
                comSource.SetNoAddRef(snap.GetRawPointer());
                return true;
            }
        }
    }

    return false;
}

void ApplicationHostCodePackageActivator::TraceWarning(
    wstring && operationName,
    vector<wstring> && codePackageNames,
    ErrorCode error)
{
    wstring cpNames;
    for (auto const & cp : codePackageNames)
    {
        cpNames = cpNames.empty() ? cp : wformatString("{0}, {1}", cpNames, cp);
    }

    if (cpNames.empty())
    {
        cpNames = L"ALL";
    }

    WriteWarning(
        TraceType,
        Root.TraceId,
        "{0} failed with Error={1}. ApplicationHostContext={2}, CodePackageContext={3}, CodePackageNames=[{4}].",
        operationName,
        error,
        this->GetHostContext(),
        this->GetCodePackageContext(),
        cpNames);
}
