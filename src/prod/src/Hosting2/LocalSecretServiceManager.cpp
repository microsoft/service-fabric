//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace Management::CentralSecretService;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceType("LocalSecretServiceManager");

// ********************************************************************************************************************
// LocalSecretServiceManager::GetSecretsAsyncOperation Implementation
//
class LocalSecretServiceManager::GetSecretsAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetSecretsAsyncOperation)

public:
    GetSecretsAsyncOperation(
        __in LocalSecretServiceManager & owner,
        Management::CentralSecretService::GetSecretsDescription & getSecretsDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        getSecretsDescription_(getSecretsDescription),
        timeoutHelper_(timeout),
        secretValues_()
    {
    }

    virtual ~GetSecretsAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "LocalSecretServiceManager::GetSecretsAsyncOperation.destructor");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out Management::CentralSecretService::SecretsDescription & secretValue)
    {
        auto thisPtr = AsyncOperation::End<GetSecretsAsyncOperation>(operation);
        secretValue = thisPtr->secretValues_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "LocalSecretServiceManager: Sending download secrets request to CentralSecretStore. Timeout : {0}",
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.Hosting.SecretStoreClient->BeginGetSecrets(
            getSecretsDescription_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            OnGetSecretsCompleted(operation, false);
        },
            thisSPtr);
        OnGetSecretsCompleted(operation, true);
    }

private:
    void OnGetSecretsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Hosting.SecretStoreClient->EndGetSecrets(operation, secretValues_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "End(OnGetSecretsCompleted): ErrorCode={0}",
                error);
        }

        TryComplete(operation->Parent, error);
    }

private:
    LocalSecretServiceManager & owner_;
    TimeoutHelper timeoutHelper_;
    Management::CentralSecretService::GetSecretsDescription & getSecretsDescription_;
    Management::CentralSecretService::SecretsDescription secretValues_;
};


// ********************************************************************************************************************
// LocalSecretServiceManager Implementation
//
LocalSecretServiceManager::LocalSecretServiceManager(
    ComponentRoot const & root, 
    __in HostingSubsystem & hosting)
    : RootedObject(root)
    , hosting_(hosting)
{

}

LocalSecretServiceManager::~LocalSecretServiceManager()
{
    WriteNoise(TraceType, Root.TraceId, "LocalSecretServiceManager.destructor");
}

ErrorCode LocalSecretServiceManager::OnOpen()
{
    /* NOP for now.
    /  V2 should implement logic to create and expose self-signed certs for authentication.
       LSS REST endpoint also needs to be configured
    */
    WriteInfo(
        TraceType,
        Root.TraceId,
        "LocalSecretServiceManager open");
    return ErrorCodeValue::Success;
}

ErrorCode LocalSecretServiceManager::OnClose()
{
    /* NOP for now.
    /  V2 should implement logic to cleanup self-signed certs generated in OnOpen.
    */
    WriteInfo(
        TraceType,
        Root.TraceId,
        "LocalSecretServiceManager closed");
    return ErrorCodeValue::Success;
}

void LocalSecretServiceManager::OnAbort()
{
    OnClose();
}

AsyncOperationSPtr LocalSecretServiceManager::BeginGetSecrets(
    Management::CentralSecretService::GetSecretsDescription & getSecretsDescription,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetSecretsAsyncOperation>(
        *this,
        getSecretsDescription,
        timeout,
        callback,
        parent);
}

ErrorCode LocalSecretServiceManager::EndGetSecrets(
    AsyncOperationSPtr const & operation, 
    __out Management::CentralSecretService::SecretsDescription & secretValues)
{
    return GetSecretsAsyncOperation::End(operation, secretValues);
}

ErrorCode LocalSecretServiceManager::ParseSecretStoreRef(
    std::vector<wstring> const & secretStoreRefs,
    _Out_ Management::CentralSecretService::GetSecretsDescription & getSecretsDescription)
{
    ErrorCode error(ErrorCodeValue::Success);
    wstring delimiter = L":";
    std::vector<Management::CentralSecretService::SecretReference> parsedSecretRefs;

    for (auto secretStoreRef: secretStoreRefs)
    {
        std::size_t startTokenPos = secretStoreRef.find_last_of(delimiter);
        if (startTokenPos != string::npos && (startTokenPos + 1) < (secretStoreRef).length())
        {
            wstring secretFqdn = (secretStoreRef).substr(0, startTokenPos);
            wstring secretVersion = (secretStoreRef).substr(startTokenPos + 1, (secretStoreRef).length() - startTokenPos);
            SecretReference parsedRef(secretFqdn, secretVersion);
            parsedSecretRefs.push_back(parsedRef);
        }
        else
        {
            error = ErrorCodeValue::InvalidArgument;
            WriteWarning(
                TraceType,
                this->Root.TraceId,
                "ParseSecretStoreRef failed. SecretStoreRef {0} should be of the form FQDN:Version. ErrorCode={1}",
                secretStoreRef,
                error);
            return error;
        }
    }

    GetSecretsDescription secretDesc(parsedSecretRefs, true);
    getSecretsDescription = move(secretDesc);

    return error;
}