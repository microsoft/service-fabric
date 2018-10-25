// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("HostedService");

#define HOSTED_SERVICE_NAME_PREFIX      L"HostedService/"
#define HOSTED_SERVICE_NAME_PREFIX_SIZE 14

class HostedService::DeactivateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    DeactivateAsyncOperation(        
        __in HostedService & HostedService,
        bool gracefulStop,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        gracefulStop_(gracefulStop),
        timeoutHelper_(timeout),        
        owner_(HostedService)
    {
    }

    static ErrorCode DeactivateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {  
        if(owner_.GetState() == owner_.Deactivated ||
            !owner_.ctrlCSpecified_)
        {//if ctrlcsender was not specified in config for this service dont deactivate it
            // This mechansism is used by fabric upgrade to launch msi
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        ErrorCode transitionResult = ErrorCodeValue::Success;
        {
            AcquireReadLock lock(owner_.childServiceLock_);
            transitionResult = owner_.TransitionToDeactivating();
        }
        if(transitionResult.IsSuccess())
        {                    
            this->DoDeactivate(thisSPtr);
        }
        else
        {            
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "HostedService failed to deactivate. ErrorCode: {0}. {1}",                
                transitionResult,
                owner_.serviceName_);

            TransitionToAborted(thisSPtr);
        }
    }

private:
    void DoDeactivate(AsyncOperationSPtr const & thisSPtr)
    {    
        ASSERT_IFNOT(owner_.hostedServiceInstance_, "HostedServiceInstance must not be null.");

        {
            AcquireWriteLock lock(owner_.Lock);      

            this->hostedServiceInstance_ = move(owner_.hostedServiceInstance_);
        }
        {
            AcquireWriteLock lock(owner_.timerLock_);
            if(owner_.updateTimer_)
            {
                owner_.updateTimer_->Cancel();
                owner_.updateTimer_.reset();
            }
        }
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DeactivateHostedServiceInstance): {0}.",
            owner_.serviceName_);

        auto operation = hostedServiceInstance_->BeginStop(
            gracefulStop_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishStopServiceInstance(operation, false); },
            thisSPtr);

        FinishStopServiceInstance(operation, true);
    }

    void FinishStopServiceInstance(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {                
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }        

        hostedServiceInstance_->EndStop(operation);

        WriteNoise(            
            TraceType,
            owner_.TraceId,
            "End(DeactivateHostedServiceInstance): {0}",
            owner_.serviceName_); 

        TransitionToDeactivated(operation->Parent);
    }

    void TransitionToDeactivated(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.TransitionToDeactivated();
        if (!error.IsSuccess())
        {
            TransitionToFailed(thisSPtr);
        }
        else
        {
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
        return;
    }

private:
    TimeoutHelper timeoutHelper_;
    HostedService & owner_;    
    HostedServiceInstanceSPtr hostedServiceInstance_;
    bool gracefulStop_;
};

class HostedService::UpdateAsyncOperation
    : public AsyncOperation,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
public:
    UpdateAsyncOperation(        
        __in HostedService & HostedService,
        wstring const & section,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , timeoutHelper_(timeout)
        , section_(section)
        , params_()
        , owner_(HostedService)
    {
    }

    UpdateAsyncOperation(        
        __in HostedService & HostedService,
        HostedServiceParameters const & params,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , timeoutHelper_(timeout)
        , section_()
        , params_(params)
        , owner_(HostedService)
    {
    }

    static ErrorCode UpdateAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateAsyncOperation>(operation);
        return thisPtr->Error.ReadValue();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {  
        ErrorCode transitionResult = owner_.TransitionToUpdating();
        if(transitionResult.IsSuccess())
        {                 
            this->DoUpdate(thisSPtr);
        }
        else
        {            
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "HostedService failed to update. ErrorCode: {0}. {1}",                
                transitionResult,
                owner_.serviceName_);
            TryComplete(thisSPtr, transitionResult);
        }
    }

private:
    void DoUpdate(AsyncOperationSPtr const & thisSPtr)
    {
        //
        // Determine which fields have been updated in the section for this hosted service.
        //
        bool requireRestart = false;
        bool secUserUpdated = false;
        bool thumbprintUpdated = false;
        bool argumentsUpdated = false;
        SecurityUserSPtr secUser;
        wstring thumbprint;
        wstring arguments;
        ErrorCode error;

        if (! section_.empty())
        {
            ASSERT_IF(! params_.ExeName.empty(), "If section_ is nonempty, request_ must be empty.");

            StringMap entries;
            FabricHostConfig::GetConfig().GetKeyValues(section_, entries);
            if(! entries.empty())
            {
                error = owner_.ActivationManager.CreateSecurityUser(section_, entries, secUser);
                if(!error.IsSuccess())
                {
                    Trace.WriteWarning(
                        TraceType,
                        owner_.TraceId,
                        "Invalid credentials specified for  HostedService {0}. Error {1}",
                        section_,
                        error);
                    return;
                }

                owner_.ActivationManager.GetHostedServiceCertificateThumbprint(entries, thumbprint);
                
                auto it = entries.find(Constants::HostedServiceParamArgs);
                if (it != entries.end())
                {
                    arguments = it->second;
                }
            }
            else
            {
                Trace.WriteInfo(
                TraceType,
                owner_.TraceId,
                "HostedService {0} being removed",
                owner_.ServiceName);

                error = owner_.TransitionToInitialized();
                if(! error.IsSuccess())
                {
                    Trace.WriteWarning(
                        TraceType,
                        owner_.TraceId,
                        "HostedService {0} failed to transition to Initialized state. CurrentState {1}",
                        owner_.ServiceName,
                        owner_.GetState());
                }

                TryComplete(thisSPtr, error);
                return;
            }
        }
        else
        {
            ASSERT_IF(params_.ExeName.empty(), "If section_ is empty, request_ may not be empty.");

            error = owner_.ActivationManager.CreateSecurityUser(params_, secUser);
            if(!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Invalid credentials specified for  HostedService {0}. Error {1}",
                    section_,
                    error);
                return;
            }

            thumbprint = params_.SslCertificateFindValue;
            arguments = params_.Arguments;
        }

        // Update service RG policy if needed
        if (owner_.IsInRgPolicyUpdate)
        {
            owner_.UpdateRGPolicy();
        }

        requireRestart = owner_.RequireServiceRestart(secUser) || owner_.IsArgumentsUpdated(arguments);
        secUserUpdated = owner_.IsSecurityUserUpdated(secUser) || requireRestart;
        thumbprintUpdated = owner_.IsThumbprintUpdated(thumbprint);
        argumentsUpdated = owner_.IsArgumentsUpdated(arguments);

        if (secUserUpdated)
        {
            owner_.securityUser_ = secUser;
        }
         
        if (thumbprintUpdated)
        {
            owner_.sslCertFindValue_ = thumbprint;
        }

        if (argumentsUpdated)
        {
            EnvironmentMap envMap;
            vector<wchar_t> envBlock = owner_.processDescription_->EnvironmentBlock;
            LPVOID penvBlock = envBlock.data();
            Environment::FromEnvironmentBlock(penvBlock, envMap);

            owner_.processDescription_ = make_unique<ProcessDescription>(
                owner_.processDescription_->ExePath,
                arguments,
                owner_.processDescription_->WorkDirectory,
                envMap,
                owner_.processDescription_->WorkDirectory,
                owner_.processDescription_->WorkDirectory,
                owner_.processDescription_->WorkDirectory,
                owner_.processDescription_->WorkDirectory,
                owner_.activatorHolder_.RootedObject.ActivateHidden,
                false,
                true,
                !owner_.ctrlCSpecified_,
                L"",
                0,
                0,
                ProcessDebugParameters(),
                owner_.RgPolicyDescription,
                ServiceModel::ServicePackageResourceGovernanceDescription(),
                owner_.GetServiceNameForCGroupOrJobObject(),
                true);
        }

        //
        // Perform the necessary updates
        //
        if (requireRestart)
        {
            ASSERT_IFNOT(owner_.hostedServiceInstance_ , "HostedServiceInstance cannot be null");
            {
                AcquireWriteLock lock(owner_.Lock);      

                this->hostedServiceInstance_ = move(owner_.hostedServiceInstance_);
            }        

            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Begin(StopHostedServiceInstance): {0} for update.",
                owner_.serviceName_);

            auto operation = hostedServiceInstance_->BeginStop(
                true,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishStopServiceInstance(operation, false); },
                thisSPtr);

            FinishStopServiceInstance(operation, true);
            return;
        }

        if (secUserUpdated)
        {
            error = owner_.UpdateCredentials();
            if (error.IsSuccess())
            {
                WriteTrace(error.ToLogLevel(),
                    TraceType,
                    owner_.TraceId,
                    "Updated credentials for HostedService {0}.",
                    owner_.ServiceName);
            }
        }

        if (error.IsSuccess() &&
            thumbprintUpdated &&
            owner_.protocol_ == ProtocolType::Enum::Https)
        {
            error = owner_.hostedServiceInstance_->ActivationManager.EndpointProvider->ConfigurePortCertificate(
                owner_.port_,
                owner_.sslCertFindValue_,
                owner_.sslCertStoreLocation_,
                owner_.sslCertFindType_);

            if (error.IsSuccess())
            {
                WriteTrace(error.ToLogLevel(),
                    TraceType,
                    owner_.TraceId,
                    "Updated thumbprint for HostedService {0}, newthumbprint {1}",
                    owner_.ServiceName,
                    thumbprint);
            }
        }

        if (error.IsSuccess())
        {

            error = owner_.TransitionToInitialized();
            if(!error.IsSuccess())
            {
                WriteWarning(TraceType,
                    owner_.TraceId,
                    "Failed to transition to Initialized state. CurrentState {0}",
                    owner_.GetState());
            }
            TryComplete(thisSPtr, error);
        }
        else
        {
            auto err = owner_.TransitionToFailed();
            WriteTrace(err.ToLogLevel(),
                TraceType,
                owner_.TraceId,
                "Failed to update HostedService {0}. Error {1}",
                owner_.ServiceName,
                err);
            TransitionToAborted(thisSPtr, error);
        }
    }

    void FinishStopServiceInstance(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {                
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }        

        hostedServiceInstance_->EndStop(operation);

        WriteNoise(            
            TraceType,
            owner_.TraceId,
            "End(DeactivateHostedServiceInstance): {0}",
            owner_.serviceName_); 

        Reinitialize(operation->Parent);
    }

    void Reinitialize(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.CreateAndStartHostedServiceInstance();
        TryComplete(thisSPtr, error);
    }

    void TransitionToAborted(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        owner_.AbortAndWaitForTerminationAsync(
            [this, error](AsyncOperationSPtr const & operation)
        {
            this->TryComplete(operation->Parent, error);
        },
            thisSPtr);
        return;
    }

private:

    TimeoutHelper timeoutHelper_;
    HostedService & owner_;    
    HostedServiceInstanceSPtr hostedServiceInstance_;
    wstring section_;
    HostedServiceParameters params_;
};



HostedService::RunStats::RunStats()
    : LastExitCode(0),
    LastActivationTime(),
    LastSuccessfulActivationTime(),
    LastExitTime(),
    LastSuccessfulExitTime(),
    ActivationFailureCount(0),
    CountinousActivationFailureCount(0),
    ExitFailureCount(0),
    ContinousExitFailureCount(0),
    ExitCount(0),
    ActivationCount(0)
{
}

void HostedService::RunStats::UpdateProcessActivationStats(bool success)
{    
    this->LastActivationTime = DateTime::Now();
    this->ActivationCount++;

    if (success)
    {
        this->LastSuccessfulActivationTime = this->LastActivationTime;
        this->CountinousActivationFailureCount = 0;
    }
    else
    {
        this->ActivationFailureCount++;
        this->CountinousActivationFailureCount++;
    } 
}

void HostedService::RunStats::UpdateProcessExitStats(DWORD exitCode)
{    
    this->LastExitCode = exitCode;
    this->LastExitTime = DateTime::Now();
    this->ExitCount++;

    if (exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode)
    {
        this->LastSuccessfulExitTime = this->LastExitTime;
        this->ContinousExitFailureCount = 0;
    }
    else
    {
        // If the HostedService is active for more than HostedServiceContinuousExitFailureResetInterval, reset the 
        // ContinousExitFailureCount to 0 even though the process exited to a non-zero value
        if(DateTime::Now() - this->LastActivationTime > FabricHostConfig::GetConfig().HostedServiceContinuousExitFailureResetInterval)
        {
            this->ContinousExitFailureCount = 0;
        }
        else
        {
            this->ContinousExitFailureCount++;
        }

        this->ExitFailureCount++;        
    }    
}

ULONG HostedService::RunStats::GetMaxContinuousFailureCount()
{
    if (this->ContinousExitFailureCount > this->CountinousActivationFailureCount)
    {
        return this->ContinousExitFailureCount;
    }
    else
    {
        return this->CountinousActivationFailureCount;
    } 
}

void HostedService::RunStats::WriteTo(__in TextWriter& w, FormatOptions const &) const
{   
    w.Write("HostedService::RunStats { ");
    w.Write("LastExitCode = {0}, ", LastExitCode);
    w.Write("LastActivationTime = {0}, ", LastActivationTime);
    w.Write("LastSuccessfulActivationTime = {0}, ", LastSuccessfulActivationTime);
    w.Write("LastExitTime = {0}, ", LastExitTime);
    w.Write("ActivationFailureCount = {0}, ", ActivationFailureCount);
    w.Write("CountinousActivationFailureCount = {0}, ", CountinousActivationFailureCount);
    w.Write("ExitFailureCount = {0} ", ExitFailureCount);
    w.Write("ContinousExitFailureCount = {0} ", ContinousExitFailureCount);
    w.Write("ExitCount = {0} ", ExitCount);
    w.Write("ActivationCount = {0} ", ActivationCount);
    w.Write("}");
}

HostedService::HostedService(
    HostedServiceActivationManagerHolder const & activatorHolder,
    std::wstring const & serviceName,
    std::wstring const & exeName,
    std::wstring const & arguments,
    std::wstring const & workingDirectory,
    std::wstring const & environment, 
    bool ctrlCSpecified, 
    std::wstring const & serviceNodeName,
    UINT port,
    wstring const & protocol,
    SecurityUserSPtr const & runAs,
    wstring const & sslCertificateFindValue,
    wstring const & sslCertStoreLocation,
    X509FindType::Enum sslCertificateFindType,
    ServiceModel::ResourceGovernancePolicyDescription const & rgPolicyDescription)
    : ComponentRoot(),
    StateMachine(Inactive),
    activatorHolder_(activatorHolder),
    serviceName_(serviceName),
    processDescription_(),
    ctrlCSpecified_(ctrlCSpecified),
    serviceNodeName_(serviceNodeName),
    port_(port),
    needPortAcls_(false),
    securityUser_(runAs),
    statsLock_(),    
    stats_(),
    hostedServiceInstance_(),
    runInterval_(TimeSpan::Zero),
    serviceId_(Guid::NewGuid().ToString()),
    sslCertFindValue_(sslCertificateFindValue),
    sslCertFindType_(sslCertificateFindType),
    sslCertStoreLocation_(sslCertStoreLocation),
    rgPolicyDescription_(rgPolicyDescription),
    rgPolicyUpdateDescription_(rgPolicyDescription),
    isInRgPolicyUpdate_(false),
    updateTimer_(),
    timerLock_(),
    childServiceLock_(),
    childServiceNames_(),
    protocol_(ProtocolType::Enum::Tcp)
{
    this->SetTraceId(activatorHolder_.Root->TraceId);
    WriteNoise(
        TraceType, 
        TraceId, 
        "HostedService.constructor: {0} ({1})", 
        static_cast<void*>(this),
        serviceName_);

    auto isContainerActivatorService = StringUtility::AreEqualCaseInsensitive(
        serviceName, 
        Constants::FabricContainerActivatorServiceName);

    EnvironmentMap envMap;
    if(isContainerActivatorService || !environment.empty())
    {
        WriteInfo(
            TraceType, 
            TraceId, 
            "HostedService Package file environment variable specified: {0} for service {1}", 
            environment,
            serviceName_);

        Environment::GetEnvironmentMap(envMap);

        envMap.insert(make_pair(FabricEnvironment::HostedServiceNameEnvironmentVariable, serviceName_));

        if (!environment.empty())
        {
            vector<wstring> envTokens;
            StringUtility::Split<wstring>(environment, envTokens, L"=");
            if (envTokens.size() > 2)
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "HostedService Package file environment variable specified: {0} for service {1} is invalid. Skip adding to env map",
                    environment,
                    serviceName_);
            }
            else
            {
                auto iter = envMap.find(envTokens[0]);
                if (iter == envMap.end())
                {
                    envMap.insert(make_pair(envTokens[0], envTokens[1]));
                }
                else
                {
                    envMap[envTokens[0]] = envMap[envTokens[1]];
                }
            }
        }
    }
    auto iter = envMap.find(Constants::FabricActivatorAddressEnvVariable);
    if(iter != envMap.end())
    {
        envMap[Constants::FabricActivatorAddressEnvVariable] = this->ActivationManager.TransportAddress;
    }
    else
    {
        envMap.insert(make_pair(Constants::FabricActivatorAddressEnvVariable, this->ActivationManager.TransportAddress));
    }
    if(!protocol.empty())
    {
        if(StringUtility::AreEqualCaseInsensitive(protocol, L"http"))
        {
            this->needPortAcls_ = true;
            protocol_ = ProtocolType::Enum::Http;
        }
        else if(StringUtility::AreEqualCaseInsensitive(protocol, L"https"))
        {
            this->needPortAcls_ = true;
            protocol_ = ProtocolType::Enum::Https;
        }
        else
        {
            WriteInfo(
                TraceType, 
                TraceId, 
                "Protocol {0} specified for HostedService {1} does not require configuration", 
                protocol,
                serviceName_);
        }
    }


    processDescription_ = make_unique<ProcessDescription>(
        exeName, 
        arguments, 
        workingDirectory,
        envMap,
        workingDirectory,
        workingDirectory,
        workingDirectory,
        workingDirectory,
        activatorHolder.RootedObject.ActivateHidden,
        false,
        true,
        !ctrlCSpecified_,
        L"",
        0,
        0,
        ProcessDebugParameters(),
        rgPolicyDescription_,
        ServiceModel::ServicePackageResourceGovernanceDescription(),
        GetServiceNameForCGroupOrJobObject(),
        true);
}

HostedService::~HostedService()
{
    WriteNoise(
        TraceType, 
        TraceId, 
        "HostedService.destructor: {0} ({1})", 
        static_cast<void*>(this),
        serviceName_);
}

void HostedService::Create(
    HostedServiceActivationManagerHolder const & activatorHolder,
    std::wstring const & serviceName,
    std::wstring const & exeName,
    std::wstring const & arguments,
    std::wstring const & workingDirectory,
    std::wstring const & environment, 
    bool ctrlCSpecified, 
    std::wstring const & serviceNodeName,
    UINT port,
    wstring const & protocol,
    SecurityUserSPtr const & runAs,
    wstring const & sslCertificateFindValue,
    wstring const & sslCertStoreLocation,
    X509FindType::Enum sslCertificateFindType,
    ServiceModel::ResourceGovernancePolicyDescription const & rgPolicyDescription,
    __out HostedServiceSPtr & hostedService)
{
    hostedService = HostedServiceSPtr(new HostedService(
        activatorHolder,
        serviceName,
        exeName,
        arguments,
        workingDirectory,
        environment,
        ctrlCSpecified,
        serviceNodeName,
        port,
        protocol,
        runAs,
        sslCertificateFindValue,
        sslCertStoreLocation,
        sslCertificateFindType,
        rgPolicyDescription));
}

ProcessDescription const & HostedService::GetProcessDescription()
{
    return *processDescription_;
}


TimeSpan HostedService::GetDueTime(RunStats stats, TimeSpan runInterval)
{    
    DateTime nowTime = DateTime::Now();
    TimeSpan dueTime = TimeSpan::Zero;

    if(stats.ActivationCount == 0)
    {
        return TimeSpan::Zero;
    }

    LONG failureCounts = stats.ContinousExitFailureCount + stats.CountinousActivationFailureCount;

    WriteNoise(
        TraceType,
        TraceId,
        "Calculating due time with FailureCount={0} and RunInterval={1}. {2}",
        failureCounts,
        runInterval,
        serviceName_);

    if (failureCounts == 0)
    {
        if(runInterval != TimeSpan::Zero)
        {            
            // run periodic task at the multiple of next period from the last execution time
            DateTime nextTime = stats.LastActivationTime + runInterval;
            while(nextTime < nowTime)
            {
                nextTime = nextTime + runInterval;
            };
            dueTime = TimeSpan::FromTicks(nextTime.Ticks - nowTime.Ticks);
        }
    }
    else
    {   
        // If there is a failure, even periodic entry point is run similar to continuous.
        // To calculate exponential back-off interval we pick 1.5 as the base so that the 
        // delay is not very large for the first 4-5 attempts.
        int64 retryTicks = (int64) (FabricHostConfig::GetConfig().ActivationRetryBackoffInterval.Ticks * failureCounts);
        auto maxRetryInterval = FabricHostConfig::GetConfig().ActivationMaxRetryInterval;
        if (retryTicks > maxRetryInterval.Ticks)
        {
            retryTicks = maxRetryInterval.Ticks;
        }

        dueTime = TimeSpan::FromTicks(retryTicks);
    }

    return dueTime;
}

wstring HostedService::GetServiceNameForCGroupOrJobObject()
{
#if !defined(PLATFORM_UNIX)
    wstring::size_type position = serviceName_.find(HOSTED_SERVICE_NAME_PREFIX);
    if (position != std::string::npos)
    {
        return serviceName_.substr(HOSTED_SERVICE_NAME_PREFIX_SIZE, std::wstring::npos);
    }
    else
    {
        return serviceName_;
    }
#else
    return L"";
#endif
}

ULONG HostedService::GetMaxContinuousFailureCount()
{
    {
        AcquireReadLock lock(statsLock_);
        return this->stats_.GetMaxContinuousFailureCount();    
    }
}

AsyncOperationSPtr HostedService::BeginActivate(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    ErrorCode error(ErrorCodeValue::Success);

    WriteNoise(
        TraceType,
        TraceId,
        "Activating HostedService {0}",
        serviceName_); 

    error = this->InitializeHostedServiceInstance();


    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        error,
        callback,
        parent);          
}

ErrorCode HostedService::EndActivate(
    AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr HostedService::BeginDeactivate(
    bool gracefulStop,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{    
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        gracefulStop, 
        timeout, 
        callback, 
        parent);
}

ErrorCode HostedService::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

AsyncOperationSPtr HostedService::BeginUpdate(
    wstring const & section,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateAsyncOperation>(
        *this,
        section,
        timeout, 
        callback, 
        parent);
}

AsyncOperationSPtr HostedService::BeginUpdate(
    HostedServiceParameters const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateAsyncOperation>(
        *this,
        params,
        timeout, 
        callback, 
        parent);
}

ErrorCode HostedService::EndUpdate(
    AsyncOperationSPtr const & operation)
{
    isInRgPolicyUpdate_ = false;
    return UpdateAsyncOperation::End(operation);
}



void HostedService::OnAbort()
{
    if(ctrlCSpecified_)
    {
        //Only terminate if CtrlSenderPath was specified in config
        HostedServiceInstanceSPtr hostedServiceInstance;

        {
            AcquireWriteLock lock(Lock);
            hostedServiceInstance = move(hostedServiceInstance_);
        }
        hostedServiceInstance->AbortAndWaitForTermination();
    }
    {
        AcquireWriteLock lock(this->timerLock_);
        if(this->updateTimer_)
        {
            this->updateTimer_->Cancel();
            this->updateTimer_.reset();
        }
    }
}

ErrorCode HostedService::InitializeHostedServiceInstance()
{
    ErrorCode transitionResult = this->TransitionToInitializing();   

    if(!transitionResult.IsSuccess())
    {
        WriteWarning(                
            TraceType,
            TraceId,
            "HostedService cannot transition to Initializing state. ErrorCode={0}. Current State={1}. {2}",           
            transitionResult,
            StateToString(this->GetState()),
            serviceName_);

        return transitionResult;
    }    
    return CreateAndStartHostedServiceInstance();
}

ErrorCode HostedService::CreateAndStartHostedServiceInstance()
{
    if(hostedServiceInstance_)
    {
        hostedServiceInstance_.reset();
    }


    hostedServiceInstance_ = make_shared<HostedServiceInstance>(
        HostedServiceHolder(*this, this->CreateComponentRoot()),
        serviceName_,
        runInterval_,
        stats_,
        statsLock_,
        RunAs); 

    hostedServiceInstance_->BeginStart(        
        [this](AsyncOperationSPtr const & operation) { this->FinishServiceInstanceStartCompleted(operation); },
        this->CreateAsyncOperationRoot());    

    auto transitionResult = this->TransitionToInitialized();
    if(!transitionResult.IsSuccess())
    {
        WriteWarning(                
            TraceType,
            TraceId,
            "HostedService {0} cannot transition to Initialized state. ErrorCode={1}. Current State={2}.",        
            serviceName_,
            transitionResult,
            StateToString(this->GetState()));

        transitionResult = this->TransitionToFailed();
    }
    return transitionResult;
}

void HostedService::FinishServiceInstanceStartCompleted(AsyncOperationSPtr const & operation)
{
    ErrorCode result = hostedServiceInstance_->EndStart(operation);
    ASSERT_IFNOT(
        result.IsSuccess(),
        "The HostedServiceInstance has to be initialized successfully.");
}


bool HostedService::RescheduleServiceActivation(DWORD exitCode)
{
    WriteTrace(
        ((exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode || exitCode == STATUS_CONTROL_C_EXIT) ? LogLevel::Info : LogLevel::Warning),
        TraceType,
        TraceId,
        "HostedServiceInstance:{0} terminated with exitcode {1}",        
        serviceName_,
        exitCode);

    bool canReschedule = true;
    {
        AcquireReadLock lock(Lock);
        if(this->hostedServiceInstance_)
        {
            canReschedule = hostedServiceInstance_->UpdateRunStats(exitCode);
        }
    }
    if(!canReschedule)
    {
        return canReschedule;
    }

    ErrorCode transitionResult = this->InitializeHostedServiceInstance();

    if(!transitionResult.IsSuccess())
    {
        WriteWarning(                
            TraceType,
            TraceId,
            "HostedService {0} cannot be restarted because of its state. ErrorCode={1}. Current State={2}.",
            serviceName_,
            transitionResult,
            StateToString(this->GetState()));
    }
    return true;
}

void HostedService::TerminateServiceInstance()
{
    if(hostedServiceInstance_)
    {
        hostedServiceInstance_->TerminateServiceInstance();
        hostedServiceInstance_.reset();
        TransitionToDeactivated();
    }
}

bool HostedService::IsRGPolicyUpdateNeeded(StringMap const & entries)
{
    if (!isInRgPolicyUpdate_)
    {
        // Get CpusetCpus rg policy
        auto itRgEntry = entries.find(Constants::HostedServiceCpusetCpus);
        if (itRgEntry != entries.end())
        {
            rgPolicyUpdateDescription_.CpusetCpus = itRgEntry->second;
        }
        else
        {
            rgPolicyUpdateDescription_.CpusetCpus = L"";
        }

        // Get CpuShares rg policy
        itRgEntry = entries.find(Constants::HostedServiceCpuShares);
        if (itRgEntry != entries.end())
        {
            if (!Config::TryParse<UINT>(rgPolicyUpdateDescription_.CpuShares, itRgEntry->second))
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Failed to parse process CpuShares limit={0}",
                    itRgEntry->second);
                return false;
            };
        }
        else
        {
            rgPolicyUpdateDescription_.CpuShares = 0;
        }

        // Get MemoryInMB rg policy
        itRgEntry = entries.find(Constants::HostedServiceMemoryInMB);
        if (itRgEntry != entries.end())
        {
            if (!Config::TryParse<UINT>(rgPolicyUpdateDescription_.MemoryInMB, itRgEntry->second))
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Failed to parse process MemoryInMB limit={0}",
                    itRgEntry->second);
                return false;
            };
        }
        else
        {
            rgPolicyUpdateDescription_.MemoryInMB = 0;
        }

        // Get MemorySwapInMB rg policy
        itRgEntry = entries.find(Constants::HostedServiceMemorySwapInMB);
        if (itRgEntry != entries.end())
        {
            if (!Config::TryParse<UINT>(rgPolicyUpdateDescription_.MemorySwapInMB, itRgEntry->second))
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Failed to parse process MemorySwapInMB limit={0}",
                    itRgEntry->second);
                return false;
            };
        }
        else
        {
            rgPolicyUpdateDescription_.MemorySwapInMB = 0;
        }

        if (rgPolicyDescription_ != rgPolicyUpdateDescription_)
        {
            isInRgPolicyUpdate_ = true;
        }
    }
    return isInRgPolicyUpdate_;
}

bool HostedService::IsRGPolicyUpdateNeeded(HostedServiceParameters const & params)
{
    if (!isInRgPolicyUpdate_ &&
        (!StringUtility::AreEqualCaseInsensitive(params.CpusetCpus, rgPolicyDescription_.CpusetCpus) ||
        params.CpuShares != rgPolicyDescription_.CpuShares ||
        params.MemoryInMB != rgPolicyDescription_.MemoryInMB ||
        params.MemorySwapInMB != rgPolicyDescription_.MemorySwapInMB))
    {
        isInRgPolicyUpdate_ = true;
        rgPolicyUpdateDescription_.CpusetCpus = params.CpusetCpus;
        rgPolicyUpdateDescription_.CpuShares = params.CpuShares;
        rgPolicyUpdateDescription_.MemoryInMB = params.MemoryInMB;
        rgPolicyUpdateDescription_.MemorySwapInMB = params.MemorySwapInMB;
    }
    return isInRgPolicyUpdate_;
}

void HostedService::UpdateRGPolicy()
{
    // Setup new RG policy values
    processDescription_->ResourceGovernancePolicy = rgPolicyUpdateDescription_;
    
    // Update new service limits
    if (ActivationManager.ProcessActivatorObj->UpdateRGPolicy(
        hostedServiceInstance_->ActivationContext,
        processDescription_).IsSuccess())
    {
        WriteInfo(
            TraceType,
            TraceId,
            "Successful updated of hosted service {0} rg policy. OldRgPolicy={1}, NewRgPolicy={2}",
            serviceName_,
            rgPolicyDescription_,
            rgPolicyUpdateDescription_);

        rgPolicyDescription_ = rgPolicyUpdateDescription_;
    }
    // If upgrade fails, rollback to the previous configuration
    else
    {
        WriteWarning(
            TraceType,
            TraceId,
            "Failed to update hosted service {0} rg policy. OldRgPolicy={1}, NewRgPolicy={2}",
            serviceName_,
            rgPolicyDescription_,
            rgPolicyUpdateDescription_);

        processDescription_->ResourceGovernancePolicy = rgPolicyDescription_;
        rgPolicyUpdateDescription_ = rgPolicyDescription_;
    }
    isInRgPolicyUpdate_ = false;
}

bool HostedService::IsUpdateNeeded(SecurityUserSPtr const & secUser, wstring const & thumbprint, wstring const & arguments)
{
    if (RequireServiceRestart(secUser))
    {
        return true;
    }

    if (IsSecurityUserUpdated(secUser))
    {
        WriteInfo(TraceType,
            this->TraceId,
            "Password update required for Hosted Service {0} and accountname {1}",
            this->serviceName_,
            this->securityUser_->AccountName);
        return true;
    }

    if (IsThumbprintUpdated(thumbprint))
    {
        WriteInfo(TraceType,
            this->TraceId,
            "Thumbprint update required for Hosted Service {0}. New thumbprint: {1}",
            this->serviceName_,
            thumbprint);
        return true;
    }

    if (IsArgumentsUpdated(arguments))
    {
        WriteInfo(TraceType,
            this->TraceId,
            "Arguments update required for Hosted Service {0}. New arguments: {1}",
            this->serviceName_,
            arguments);
        return true;
    }

    return false;
}

bool HostedService::IsSecurityUserUpdated(SecurityUserSPtr const & secUser)
{
    if(this->securityUser_ &&
        securityUser_->IsPasswordUpdated(secUser))
    {
        return true;
    }
    return false;
}

bool HostedService::IsThumbprintUpdated(wstring const & thumbprint)
{
    if (this->sslCertFindValue_ != thumbprint)
    {
        return true;
    }
    return false;
}

bool HostedService::IsArgumentsUpdated(wstring const & arguments)
{
    if (this->processDescription_->Arguments != arguments)
    {
        return true;
    }
    return false;
}

bool HostedService::RequireServiceRestart(SecurityUserSPtr const & secUser)
{
    if(!this->securityUser_ && !secUser)
    {
        return false;
    }
    else if(!this->securityUser_ || 
        !secUser)
    {
        return true;
    }

    if((!secUser->AccountName.empty() &&
        !StringUtility::AreEqualCaseInsensitive(securityUser_->AccountName, secUser->AccountName)) ||
        securityUser_->AccountType != secUser->AccountType)
    {
        WriteInfo(TraceType,
            this->TraceId,
            "Account change detected for Hosted Service {0}. Original accountname {1} accounttype {2}. New accountname {3} accounttype {4}",
            this->serviceName_,
            this->securityUser_->AccountName,
            this->securityUser_->AccountType,
            secUser->AccountName,
            secUser->AccountType);
        return true;
    }
    return false;
}

ErrorCode HostedService::UpdateCredentials()
{
    ErrorCode error;
    if(this->securityUser_->AccountType == SecurityPrincipalAccountType::DomainUser)
    {
        auto domUser = shared_ptr<DomainUser>(dynamic_pointer_cast<DomainUser>(securityUser_));
        error = domUser->UpdateCachedCredentials();
        if(!error.IsSuccess())
        {
            WriteWarning(TraceType,
                TraceId,
                "Failed to update cached credentials for Hosted Service {0}. Error {1}",
                this->serviceName_,
                error);
        }
    }
    else
    {
        WriteInfo(TraceType,
            TraceId,
            "Account {0} with AccountType {1} for HostedService {2} does not require cached credentials update",
            this->securityUser_->AccountName,
            this->securityUser_->AccountType,
            this->serviceName_);

        error = ErrorCode(ErrorCodeValue::Success);
    }
    return error;
}

bool HostedService::CanScheduleUpdate()
{
    uint64 currentState = this->GetState();
    if(currentState == HostedService::Deactivating ||
        ((HostedService::GetTerminalStatesMask () & currentState) != 0))
    {
        return false;
    }
    return true;
}

ErrorCode HostedService::AddChildService(wstring const & childServiceName)
{
    AcquireWriteLock loc(childServiceLock_);
    auto currentState = this->GetState();
    if (currentState == Deactivating ||
        currentState == Deactivated ||
        currentState == Aborted)
    {
        return ErrorCodeValue::InvalidState;
    }
    childServiceNames_.push_back(childServiceName);
    return ErrorCodeValue::Success;
}

void HostedService::ClearChildServices(vector<wstring> & childServiceNames)
{
    AcquireWriteLock lock(childServiceLock_);
    for (auto it = childServiceNames_.begin(); it != childServiceNames_.end(); it++)
    {
        childServiceNames.push_back(*it);
    }
    childServiceNames_.clear();
}


