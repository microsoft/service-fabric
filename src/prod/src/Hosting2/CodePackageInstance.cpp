// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("CodePackageInstance");

class CodePackageInstance::StartAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    StartAsyncOperation(
        CodePackageInstance & codePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        codePackageInstance_(codePackageInstance)
    {
    }

    static ErrorCode StartAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<StartAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode transitionResult = codePackageInstance_.TransitionToStarting();

        if(transitionResult.IsSuccess())
        {
            this->DoStartProcess(thisSPtr);
        }
        else
        {
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} failed to start. ErrorCode={1}. {2}:{3}",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            TryComplete(thisSPtr, transitionResult);
        }
    }

private:
    void DoStartProcess(AsyncOperationSPtr const & thisSPtr)
    {         
        if (codePackageInstance_.EntryPoint.EntryPointType == EntryPointType::None)
        {
            ErrorCode errorCode = ErrorCode(ErrorCodeValue::Success);
            errorCode = this->TransitionToStarted(errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }
        else
        {
            codePackageInstance_.ResetCodePackageActivationId();

            codePackageInstance_.codePackageRuntimeInformation_.reset();

            WriteNoise(
                TraceType,
                codePackageInstance_.TraceId,
                "Begin(ActivateCodePackage): EntryPoint={0}, {1}:{2}:{3}",
                codePackageInstance_.EntryPoint,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_,
                codePackageInstance_.InstanceId);

            auto operation = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->BeginActivateCodePackage(
                dynamic_pointer_cast<CodePackageInstance>(codePackageInstance_.shared_from_this()),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishStartCompleted(operation, false); },
                thisSPtr);

            FinishStartCompleted(operation, true);
        }
    }

    void FinishStartCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation;
        CodePackageActivationIdUPtr activationIdUPtr;
        ErrorCode errorCode = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->EndActivateCodePackage(
            operation,
            activationIdUPtr,
            codePackageRuntimeInformation);

        {
            AcquireWriteLock lock(codePackageInstance_.Lock);
            codePackageInstance_.activationId_ = move(activationIdUPtr);
        }

        codePackageInstance_.codePackageRuntimeInformation_ = codePackageRuntimeInformation;

        WriteTrace(
            errorCode.ToLogLevel(),
            TraceType,
            codePackageInstance_.TraceId,
            "End(ActivateCodePackage): ErrorCode={0}, EntryPoint={1}, {2}:{3}.",
            errorCode,
            codePackageInstance_.EntryPoint,
            codePackageInstance_.codePackageHolder_.RootedObject.id_,
            codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

        ErrorCode transitionResult = this->TransitionToStarted(errorCode);

        if(!transitionResult.IsSuccess() && errorCode.IsSuccess())
        {
           errorCode = transitionResult;
        }

        TryComplete(operation->Parent, errorCode);
    }

    ErrorCode TransitionToStarted(ErrorCode startedResult)
    {
        ErrorCode transitionResult;
        if(startedResult.IsSuccess())
        {
            transitionResult = codePackageInstance_.TransitionToStarted();

            WriteInfo(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} has started. {1}:{2}",
                codePackageInstance_.EntryPoint,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);
        }
        else
        {
            transitionResult = codePackageInstance_.TransitionToScheduling();

            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} has failed to start. ErrorCode={1}, CurrentState={2}, {3}:{4}",
                codePackageInstance_.EntryPoint,
                startedResult,
                codePackageInstance_.StateToString(codePackageInstance_.GetState()),
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);
        }

        if(!transitionResult.IsSuccess())
        {
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} has failed to transition to the right state while starting. ErrorCode: {1}=CurrentState={2}. {3}:{4}",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.StateToString(codePackageInstance_.GetState()),
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            codePackageInstance_.TransitionToFailed();
        }

        return transitionResult;
    }

private:
    TimeoutHelper timeoutHelper_;
    CodePackageInstance & codePackageInstance_;
};

class CodePackageInstance::StopAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    StopAsyncOperation(
        CodePackageInstance & codePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        codePackageInstance_(codePackageInstance)
    {
    }

    static void StopAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<StopAsyncOperation>(operation);
        thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {        
        if(codePackageInstance_.GetState() == codePackageInstance_.Stopped)
        {            
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        ErrorCode transitionResult = codePackageInstance_.TransitionToStopping();
        if(transitionResult.IsSuccess())
        {
            this->DoStopProcess(thisSPtr);
        }
        else
        {
            // Error in transition to Stopping state
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} failed to stop. ErrorCode={1}, {2}:{3}",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            TransitionToAborted(thisSPtr);
        }
    }

private:
    void DoStopProcess(AsyncOperationSPtr const & thisSPtr)
    {
        this->codePackageInstance_.CleanupTimer();

        CodePackageActivationId activationId;

        codePackageInstance_.GetCodePackageActivationId(activationId);

        if ((codePackageInstance_.EntryPoint.EntryPointType == EntryPointType::None) || 
            (activationId.IsEmpty()))
        {
            TransitionToStopped(thisSPtr);
            return;
        }
        else
        {
            WriteNoise(
                TraceType,
                codePackageInstance_.TraceId,
                "Begin(DeactivateCodePackage): EntryPoint={0}, {1}:{2}",
                codePackageInstance_.EntryPoint,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            auto operation = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->BeginDeactivateCodePackage(
                codePackageInstance_.CodePackageInstanceId,
                activationId,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishStopCompleted(operation, false); },
                thisSPtr);

            FinishStopCompleted(operation, true);
        }
    }

    void FinishStopCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto errorCode = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->EndDeactivateCodePackage(operation);

        WriteTrace(
            errorCode.ToLogLevel(),
            TraceType,
            codePackageInstance_.TraceId,
            "End(DeactivateCodePackage): ErrorCode={0}, EntryPoint={1}, {2}:{3}",
            errorCode,
            codePackageInstance_.EntryPoint,
            codePackageInstance_.codePackageHolder_.RootedObject.id_,
            codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

        if (errorCode.IsSuccess())
        {
            TransitionToStopped(operation->Parent);
        }
        else
        {
            TransitionToFailed(operation->Parent);
        }

    }

    void TransitionToStopped(AsyncOperationSPtr const & thisSPtr)
    {
        auto transitionResult = codePackageInstance_.TransitionToStopped();
        if (transitionResult.IsSuccess())
        {
            codePackageInstance_.CodePackageObj.Hosting.FabricRuntimeManagerObj->OnCodePackageClosed(
                codePackageInstance_.Context.CodePackageInstanceId,
                codePackageInstance_.Context.CodePackageInstanceSeqNum);

            codePackageInstance_.ResetCodePackageActivationId();

            codePackageInstance_.stats_.UpdateProcessExitStats(ProcessActivator::ProcessDeactivateExitCode, false);

            {
                AcquireWriteLock lock(codePackageInstance_.Lock);
                codePackageInstance_.codePackageRuntimeInformation_.reset();
            }

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} has failed to tranistion to Stopped state. ErrorCode: {1}. Current state is {2}.",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.StateToString(codePackageInstance_.GetState()));

            TransitionToFailed(thisSPtr);
        }
    }

    void TransitionToFailed(AsyncOperationSPtr const & thisSPtr)
    {
        codePackageInstance_.TransitionToFailed().ReadValue();
        TransitionToAborted(thisSPtr);
    }

    void TransitionToAborted(AsyncOperationSPtr const & thisSPtr)
    {
        codePackageInstance_.AbortAndWaitForTerminationAsync(
            [this](AsyncOperationSPtr const & operation)
        {
            this->TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        },
            thisSPtr);
        return;
    }

private:
    TimeoutHelper timeoutHelper_;
    CodePackageInstance & codePackageInstance_;
};

class CodePackageInstance::GetContaninerInfoAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    GetContaninerInfoAsyncOperation(
        CodePackageInstance & codePackageInstance,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        codePackageInstance_(codePackageInstance),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs)
    {
    }

    static ErrorCode GetContaninerInfoAsyncOperation::End(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
    {
        auto thisPtr = AsyncOperation::End<GetContaninerInfoAsyncOperation>(operation);
        if (thisPtr->Error.ReadValue() == ErrorCodeValue::Success)
        {
            containerInfo = move(thisPtr->containerInfo_);
        }
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {        
        if(codePackageInstance_.GetState() == codePackageInstance_.Stopped)
        {            
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        this->DoGetContainerInfo(thisSPtr);
    }

private:
    void DoGetContainerInfo(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageActivationId activationId;

        codePackageInstance_.GetCodePackageActivationId(activationId);
        if ((activationId.IsEmpty()))
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::NotFound, L"CodePackageActivationId Not Found"));
            return;
        }

        if ((codePackageInstance_.EntryPoint.EntryPointType == EntryPointType::ContainerHost))
        {
            WriteNoise(
                TraceType,
                codePackageInstance_.TraceId,
                "Begin(GetContainerInfo): EntryPoint={0}, {1}:{2}",
                codePackageInstance_.EntryPoint,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            auto operation = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->BeginGetContainerInfo(
                codePackageInstance_.CodePackageInstanceId,
                activationId,
                containerInfoType_,
                containerInfoArgs_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishGetContainerInfoCompleted(operation, false); },
                thisSPtr);

            FinishGetContainerInfoCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidOperation, L"Not a ContainerHost EntryPoint"));
            return;
        }
    }

    void FinishGetContainerInfoCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {            
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto errorCode = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->EndGetContainerInfo(operation, containerInfo_);

        WriteTrace(
            errorCode.ToLogLevel(),
            TraceType,
            codePackageInstance_.TraceId,
            "End(GetContainerInfo): ErrorCode={0}, EntryPoint={1}, {2}:{3}",
            errorCode,
            codePackageInstance_.EntryPoint,
            codePackageInstance_.codePackageHolder_.RootedObject.id_,
            codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

        TryComplete(operation->Parent, errorCode);
    }

private:
    TimeoutHelper timeoutHelper_;
    CodePackageInstance & codePackageInstance_;
    wstring containerInfoType_;
    wstring containerInfoArgs_;
    wstring containerInfo_;
};

class CodePackageInstance::UpdateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    UpdateAsyncOperation(
        CodePackageContext const & updatedContext,
        CodePackageInstance & codePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        timeoutHelper_(timeout),
        codePackageInstance_(codePackageInstance),
        updatedContext_(updatedContext)
    {
    }

    static ErrorCode UpdateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {                
        ErrorCode transitionResult = codePackageInstance_.TransitionToUpdating();
        if(transitionResult.IsSuccess())
        {
            DoUpdateContext(thisSPtr);
        }
        else
        {
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} failed to update. ErrorCode={1}, {2}:{3}",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            TryComplete(thisSPtr, transitionResult);
        }
    }

private:
    void DoUpdateContext(AsyncOperationSPtr const & thisSPtr)
    {
        if(codePackageInstance_.EntryPoint.EntryPointType == EntryPointType::None)
        {
            ErrorCode errorCode = ErrorCode(ErrorCodeValue::Success);
            errorCode = this->TransitionToStarted(errorCode);
            TryComplete(thisSPtr, errorCode);
            return;
        }
        else
        {
            WriteInfo(
                TraceType,
                codePackageInstance_.TraceId,
                "Begin(UpdateCodePackageContext): EntryPoint={0}, {1}:{2}",
                codePackageInstance_.EntryPoint,
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

            CodePackageActivationId activationId;
            codePackageInstance_.GetCodePackageActivationId(activationId);

            auto operation = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->BeginUpdateCodePackageContext(
                updatedContext_,
                activationId,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishUpdateCompleted(operation, false); },
                thisSPtr);

            FinishUpdateCompleted(operation, true);
        }
    }

    void FinishUpdateCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        ErrorCode errorCode = codePackageInstance_.CodePackageObj.Hosting.ApplicationHostManagerObj->EndUpdateCodePackageContext(operation);
        WriteTrace(
            errorCode.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            codePackageInstance_.TraceId,
            "End(UpdateCodePackageContext): ErrorCode={0}, EntryPoint={1}, {2}:{3}",
            errorCode,
            codePackageInstance_.EntryPoint,
            codePackageInstance_.codePackageHolder_.RootedObject.id_,
            codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);

        ErrorCode transitionResult = this->TransitionToStarted(errorCode);
        if(!transitionResult.IsSuccess())
        {
            errorCode = transitionResult;
        }

        TryComplete(operation->Parent, errorCode);
    }

    ErrorCode TransitionToStarted(ErrorCode updatedErrorCode)
    {
        ErrorCode transitionResult;
        if(updatedErrorCode.IsSuccess())
        {
            transitionResult = codePackageInstance_.TransitionToStarted();
        }
        else
        {
            transitionResult = codePackageInstance_.TransitionToFailed();
        }

        if(!transitionResult.IsSuccess())
        {
            WriteWarning(
                TraceType,
                codePackageInstance_.TraceId,
                "CodePackageInstance with {0} has failed to transition to the right state while updating. ErrorCode={1}, CurrentState={2}, {3}:{4}",
                codePackageInstance_.EntryPoint,
                transitionResult,
                codePackageInstance_.StateToString(codePackageInstance_.GetState()),
                codePackageInstance_.codePackageHolder_.RootedObject.id_,
                codePackageInstance_.codePackageHolder_.RootedObject.instanceId_);
        }

        return transitionResult;
    }

private:
    CodePackageContext const & updatedContext_;
    TimeoutHelper timeoutHelper_;
    CodePackageInstance & codePackageInstance_;
};

class CodePackageInstance::ApplicationHostCodePackageOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationHostCodePackageOperation(
        __in CodePackageInstance & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , parentCodepackage_(owner.codePackageHolder_.RootedObject)
        , request_(request)
    {
    }

    static ErrorCode ApplicationHostCodePackageOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackageOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto state = owner_.GetState();
        if (state == CodePackageInstance::Aborted ||
            state >= CodePackageInstance::Stopping)
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ApplicationHostCodePackageOperation: Ignoring {0} as current state is '{1}'.",
                request_,
                owner_.StateToString(state));

            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto operation = parentCodepackage_.BeginApplicationHostCodePackageOperation(
            request_,
            owner_.InstanceId,
            [this](AsyncOperationSPtr const & operation)
            {
                OnApplicationHostCodePackageOperationCompleted(operation, false);
            },
            thisSPtr);

        OnApplicationHostCodePackageOperationCompleted(operation, true);
    }

private:

    void OnApplicationHostCodePackageOperationCompleted(
        AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = parentCodepackage_.EndApplicationHostCodePackageOperation(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnApplicationHostCodePackageOperationCompleted: ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
    }

private:
    CodePackageInstance & owner_;
    CodePackage & parentCodepackage_;
    ApplicationHostCodePackageOperationRequest request_;
};

CodePackageInstance::CodePackageInstance(
    CodePackageHolder const & codePackageHolder,
    EntryPointDescription const & entryPoint,
    StartCallback const & entryPointStartedCallback,
    TerminationCallback const & entryPointTerminatedCallback,
    bool isShared,
    bool isSetupEntryPoint,
    bool isActivator,
    TimeSpan runInterval,
    CodePackage::RunStats & stats,
    wstring const & runAsId)
    : ComponentRoot(),
    StateMachine(Created),
    codePackageHolder_(codePackageHolder),
    entryPoint_(entryPoint),
    entryPointStartedCallback_(entryPointStartedCallback),
    entryPointTerminatedCallback_(entryPointTerminatedCallback),
    isShared_(isShared),
    isSetupEntryPoint_(isSetupEntryPoint),
    isActivator_(isActivator),
    runInterval_(runInterval),
    stats_(stats),
    runAsId_(runAsId),
    codePackageRuntimeInformation_(),
    dueTime_(TimeSpan::Zero),
    instanceId_(codePackageHolder.Value.Hosting.GetNextSequenceNumber())
{
    ASSERT_IF(
        isSetupEntryPoint_ && isActivator_,
        "A setup entry point cannot be an activator code package.");

    this->SetTraceId(codePackageHolder_.Root->TraceId);

    WriteNoise(
        TraceType, 
        TraceId, 
        "CodePackageInstance.constructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        codePackageHolder_.RootedObject.id_,
        codePackageHolder_.RootedObject.instanceId_);
}

CodePackageInstance::~CodePackageInstance()
{
    WriteNoise(
        TraceType, 
        TraceId, 
        "CodePackageInstance.destructor: {0} ({1}:{2})", 
        static_cast<void*>(this),
        codePackageHolder_.RootedObject.id_,
        codePackageHolder_.RootedObject.instanceId_);
}

CodePackageContext const & CodePackageInstance::get_CodePackageContext() const
{ 
    return this->CodePackageObj.Context; 
}

CodePackageInstanceIdentifier const & CodePackageInstance::get_CodePackageInstanceId() const
{ 
    return this->CodePackageObj.CodePackageInstanceId; 
}

wstring const & CodePackageInstance::get_Version() const
{ 
    return this->CodePackageObj.Version; 
}

wstring const & CodePackageInstance::get_Name() const
{ 
    return this->CodePackageObj.Description.Name; 
}

AsyncOperationSPtr CodePackageInstance::BeginStart(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    ErrorCode transitionResult = this->TransitionToScheduling();
    if(transitionResult.IsSuccess())
    {
        this->ScheduleStart();
    }
    else
    {
        WriteWarning(
            TraceType,
            TraceId,
            "CodePackageInstance with {0} failed to transition to Scheduling state. CurrentState={1}, ErrorCode={2}, {3}:{4}.",
            EntryPoint,
            StateToString(this->GetState()),
            transitionResult,
            this->CodePackageObj.id_,
            this->CodePackageObj.instanceId_);
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        transitionResult,
        callback,
        parent);
}

ErrorCode CodePackageInstance::EndStart(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackageInstance::BeginStop(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<StopAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

void CodePackageInstance::EndStop(AsyncOperationSPtr const & operation)
{
    StopAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackageInstance::BeginUpdateContext(
    CodePackageContext const & updatedContext, 
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<UpdateAsyncOperation>(
        updatedContext,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode CodePackageInstance::EndUpdateContext(AsyncOperationSPtr const & operation)
{
    return UpdateAsyncOperation::End(operation);
}

AsyncOperationSPtr CodePackageInstance::BeginGetContainerInfo(
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<GetContaninerInfoAsyncOperation>(
        *this,
        containerInfoType,
        containerInfoArgs,
        timeout,
        callback,
        parent);
}

ErrorCode CodePackageInstance::EndGetContainerInfo(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
{
    return GetContaninerInfoAsyncOperation::End(operation, containerInfo);
}

AsyncOperationSPtr CodePackageInstance::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & request,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageOperation>(
        *this,
        request,
        callback,
        parent);
}

ErrorCode CodePackageInstance::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageOperation::End(operation);
}

void CodePackageInstance::ScheduleStart()
{
    this->CleanupTimer();

    dueTime_ = this->codePackageHolder_.RootedObject.GetDueTime(this->GetRunStats(), this->runInterval_);

    auto thisSPtr = this->CreateAsyncOperationRoot();
    runTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](Common::TimerSPtr const &) { this->OnScheduledStart(thisSPtr); }, true);	

    ErrorCode transitionResult = this->TransitionToScheduled();

    if(!transitionResult.IsSuccess())
    {
        this->CleanupTimer();

        WriteWarning(
            TraceType,
            TraceId,
            "CodePackageInstance with {0} failed to transition to Scheduled state. CurrentState={1}, ErrorCode={2}, {3}:{4}",
            EntryPoint,
            StateToString(this->GetState()),
            transitionResult,
            this->CodePackageObj.id_,
            this->CodePackageObj.instanceId_);
    }
    else
    {
        WriteInfo(
            TraceType,
            TraceId,
            "CodePackageInstance with {0} is scheduled to run at {1}. {2}:{3}",
            EntryPoint,
            dueTime_,
            this->CodePackageObj.id_,
            this->CodePackageObj.instanceId_);

        runTimer_->Change(dueTime_);
    }
}

void CodePackageInstance::OnScheduledStart(AsyncOperationSPtr const & parent)
{    
    this->CleanupTimer();

    auto  activationtimeout = HostingConfig::GetConfig().ActivationTimeout;

#if defined(PLATFORM_UNIX)
    // For clear containers download happens as part of activation.
    // This is a short term fix for download for ClearContainers.
    if (this->EntryPoint.EntryPointType == EntryPointType::ContainerHost
        && this->EnvContext != nullptr && this->EnvContext->ContainerGroupIsolated)
    {
        activationtimeout = HostingConfig::GetConfig().ContainerImageDownloadTimeout + HostingConfig::GetConfig().ActivationTimeout;
    }
#endif

    auto operation = AsyncOperation::CreateAndStart<StartAsyncOperation>(
        *this,
        activationtimeout,
        [this](AsyncOperationSPtr const & operation) { this->FinishStartCompleted(operation, false); },
        parent);

    FinishStartCompleted(operation, true);
}

void CodePackageInstance::FinishStartCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynhronously)
    {
        return;
    }

    ErrorCode errorCode = StartAsyncOperation::End(operation);

    this->stats_.UpdateProcessActivationStats(errorCode);
    this->entryPointStartedCallback_(errorCode, this->instanceId_);

    if(!errorCode.IsSuccess())
    {
        auto state = this->GetState();
        if (state == CodePackageInstance::Aborted || state > CodePackageInstance::Updating)
        {
            WriteWarning(
                TraceType,
                TraceId,
                "CodePackageInstance with {0} is not running and cannot be restarted because of its state. CurrentState={1}. ErrorCode={2}. {3}:{4}",
                this->EntryPoint,
                this->StateToString(this->GetState()),
                errorCode,
                this->CodePackageObj.id_,
                this->CodePackageObj.instanceId_);

            this->TransitionToFailed().ReadValue();
        }
        else
        {
            ScheduleStart();
        }
    }
}

void CodePackageInstance::OnEntryPointTerminated(CodePackageActivationId const & activationId, DWORD exitCode, bool ignoreReporting)
{
    // TODO: Bug check activationId value and ignore any stale notification
    // since this code needs to run outside of the state machine, the use of activationId must be protected
    // here and elsewhere in the code as well. Since, operations such as TerminateExternally can be in-flight in parallel and need to access activationId_.
    UNREFERENCED_PARAMETER(activationId);

    ErrorCode transitionError = this->TransitionToStopping();
    if(transitionError.IsSuccess())
    {
        this->ResetCodePackageActivationId();

        this->TransitionToStopped();

        this->stats_.UpdateProcessExitStats(exitCode, ignoreReporting);
    }
    else
    {
        auto state = this->GetState();
        if (state == CodePackageInstance::Stopped ||
            state == CodePackageInstance::Stopping)
        {
            WriteNoise(
                TraceType,
                TraceId,
                "CodePackageInstance with {0} transition to stopping failed. CurrentState={1}. ErrorCode={2}. {3}:{4}",
                this->EntryPoint,
                this->StateToString(state),
                transitionError,
                this->CodePackageObj.id_,
                this->CodePackageObj.instanceId_);
        }
        else
        {
            this->Abort();
        }
    }    

    this->entryPointTerminatedCallback_(exitCode, this->instanceId_);
}

void CodePackageInstance::OnHealthCheckStatusChanged(
    CodePackageActivationId const & activationId,
    ContainerHealthStatusInfo const & healthStatusInfo)
{
    UNREFERENCED_PARAMETER(activationId);

    this->stats_.UpdateDockerHealthCheckStatusStats(healthStatusInfo, this->instanceId_);
}

void CodePackageInstance::OnAbort()
{    
    this->CleanupTimer();

    CodePackageActivationId activationId;
    this->GetCodePackageActivationId(activationId);

    if (!activationId.IsEmpty())
    {
        this->CodePackageObj.Hosting.ApplicationHostManagerObj->TerminateCodePackage(activationId);

        this->ResetCodePackageActivationId();

        // Why this? We are aborting it. It is not terminating by itself.
        stats_.UpdateProcessExitStats(ProcessActivator::ProcessAbortExitCode, false);
    }

    this->CodePackageObj.Hosting.FabricRuntimeManagerObj->OnCodePackageClosed(
        this->Context.CodePackageInstanceId,
        this->Context.CodePackageInstanceSeqNum);
}

void CodePackageInstance::CleanupTimer()
{
    AcquireWriteLock lock(this->Lock);

    if (runTimer_)
    {
        runTimer_->Cancel();
        runTimer_.reset();
    }
}

void CodePackageInstance::GetCodePackageActivationId(__out CodePackageActivationId & activationId)
{
    {
        AcquireReadLock lock(this->Lock);
        if (activationId_)
        {
            activationId = *activationId_;
        }
    }
}

void CodePackageInstance::ResetCodePackageActivationId()
{
    {
        AcquireWriteLock lock(this->Lock);
        activationId_.reset();
    }
}

ErrorCode CodePackageInstance::TerminateExternally()
{
    CodePackageActivationId activationId;
    this->GetCodePackageActivationId(activationId);

    if (activationId.IsEmpty())
    {
        /*
        This needs to be checked in 2 cases below:
        Scenario1:
        1. Guest exe sees a crash.
        2. FabricTypeHost sees a crash at the same time.
        3. In CodePackageInstance::OnEntryPointTerminated we transition to stopped state, reset the activationId and update the stats.
        4. So, the termination of GuestExe will never go through as activationId is reset.

        Scenario2: If there are multiple Guest exe
        1. If one of the GuestExe crash
        2. The Guest Exe will end up terminating the FabricTypeHost and activationId is gone.
        3. If during the same time other Guest exe crashes, they won't restart the FabricTypeHost without this check.
        4. If we return NotFound then the GuestExe will keep retrying the Termination until it is able to do so and then schedule it's own restart.

        Both these events are not likely but they may happen.

        States in which ActivationId can be empty:
        1. Stopped. Either it got deactivated or saw a failover(will go to scheduling state).
           It can be in Stopped because if 2 reasons:
           a) It went down i.e OnEntryPointTerminated so this will be scheduled again.
           b) Code Package got deactivated. So, when we retry at VersionedServicePackage level to terminate FabricTypeHost,
              the VersionedServicePackage should not be in Opened state or have no code packages.
        2. Scheduled/Scheduling. If it is already scheduled, then it saw a failover and the activationId was reset.
        3. Starting. So, if has not started yet and Fabric has sent a message to FabricHost. Once we transition to Started we will see a valid activationId.

        We will wait for the CodePackageInstance to go to Started state again.
        */

        uint64 currentState = this->GetState();

        //TODO: Can be changed to BeginWaitForTransition to STarted stated. In this case, we won't need the due time in VersionedSP.
        if (currentState == Scheduling || currentState == Scheduled || currentState == Stopped || currentState == Starting)
        {
            return ErrorCodeValue::NotFound;
        }

        return ErrorCodeValue::Success;
    }

    return this->CodePackageObj.Hosting.ApplicationHostManagerObj->TerminateCodePackageExternally(activationId);
}

void CodePackageInstance::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDesc)
{
    auto state = this->GetState();
    if (state == CodePackageInstance::Aborted ||
        state >= CodePackageInstance::Stopping)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "SendDependentCodePackageEvent: Ignoring Event={0} as CurrentState={1}, InstanceId={2}. CodePackageInstanceId={3}.",
            eventDesc,
            this->StateToString(state),
            this->InstanceId,
            this->CodePackageInstanceId);
        return;
    }

    CodePackageActivationId activationId;
    this->GetCodePackageActivationId(activationId);

    if (activationId.IsEmpty())
    {
        WriteNoise(
            TraceType,
            TraceId,
            "SendDependentCodePackageEvent: Failed to get ActivationId. Event={0}, CurrentState={1}, InstanceId={2}. CodePackageInstanceId={3}.",
            eventDesc,
            this->StateToString(state),
            this->InstanceId,
            this->CodePackageInstanceId);
        return;
    }

    this->ApplicationHostManagerObj->SendDependentCodePackageEvent(eventDesc, activationId);
}

CodePackageEntryPoint CodePackageInstance::GetCodePackageEntryPoint()
{    
    wstring exePath = L"";
    int64 processId = -1;
    ServiceModel::CodePackageEntryPointStatistics stats;
    EntryPointStatus::Enum status;
    DateTime nextActivationTime = DateTime::Zero;
    {
        AcquireReadLock lock(this->Lock);        
        stats = stats_.Stats;        

        uint64 currentState = this->GetState_CallerHoldsLock();
        if(currentState == Scheduled)
        {
            status = EntryPointStatus::Pending;
            if (stats.LastExitTime == DateTime::Zero)
            {
                nextActivationTime = DateTime::Now();
            }
            else
            {
                nextActivationTime = stats.LastActivationTime + dueTime_;
            }
        }
        else if(currentState == Starting)
        {
            status = EntryPointStatus::Starting;
        }
        else if(currentState == Started || currentState == Updating)
        {
            status = EntryPointStatus::Started;
            ASSERT_IFNOT(codePackageRuntimeInformation_, "CodePackage RuntimeInfomration should be set if in Started or Updating state");
            exePath = codePackageRuntimeInformation_->ExePath;
            processId = codePackageRuntimeInformation_->ProcessId;
        }
        else if(currentState == Stopping)
        {
            status = EntryPointStatus::Stopping;
        }
        else if(currentState == Created || currentState == Scheduling || currentState == Stopped || currentState == Aborted || currentState == Failed)
        {
            status = EntryPointStatus::Stopped;
        }
        else
        {
            Assert::CodingError("CodePackageInstance: State={0} should be mapped to FABRIC_ENTRY_POINT_STATUS", currentState);
        }
    }    

    return CodePackageEntryPoint(
        exePath,
        processId,
        this->RunAsId,
        nextActivationTime,
        status,
        stats,
        instanceId_);
}

CodePackageEntryPointStatistics CodePackageInstance::GetRunStats()
{
    return stats_.Stats;
}

DateTime CodePackageInstance::GetLastActivationTime()
{
    {
        AcquireReadLock lock(this->Lock);
        return stats_.Stats.LastActivationTime;
    }
}