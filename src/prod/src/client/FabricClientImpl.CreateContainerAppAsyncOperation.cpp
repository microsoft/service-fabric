// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Client;
using namespace Management::ClusterManager;
using namespace Transport;  

StringLiteral const TraceComponent("Client.CreateContainerApplication");

FabricClientImpl::CreateContainerAppAsyncOperation::CreateContainerAppAsyncOperation(
    FabricClientImpl &client,
    wstring const &deploymentName,
    wstring const &composeFilePath,
    wstring const &overridesFilepath,
    wstring const &repositoryUserName,
    wstring const &repositoryPassword,
    bool isPasswordEncrypted,
    Common::ActivityId const& activityId,
    TimeSpan const timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
        : ClientAsyncOperationBase(
                client, 
                FabricActivityHeader(activityId), 
                ErrorCode(ErrorCodeValue::Success), 
                timeout, 
                callback, 
                parent)
        , deploymentName_(deploymentName)
        , composeFilePath_(composeFilePath)
        , overridesFilePath_(overridesFilepath)
        , repositoryUserName_(repositoryUserName)
        , repositoryPassword_(repositoryPassword)
        , isPasswordEncrypted_(isPasswordEncrypted)
        , composeFileContents_()
        , overridesFileContents_()
{
}

void FabricClientImpl::CreateContainerAppAsyncOperation::OnStartOperation(AsyncOperationSPtr const &thisSPtr)
{
    File composeFile;
    auto error = composeFile.TryOpen(
        composeFilePath_,
        FileMode::Open,
        FileAccess::Read,
        FileShare::Read,
        FileAttributes::SequentialScan);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0}: Failed to open ComposeFile '{1}', Error: {2}", 
            this->ActivityId,
            composeFilePath_,
            error);

        TryComplete(thisSPtr, error);
        return;
    }

    File overridesFile;
    if (!overridesFilePath_.empty())
    {
        error = overridesFile.TryOpen(
            overridesFilePath_,
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read,
            FileAttributes::SequentialScan);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0}: Failed to open Overrides file - filename : '{1}', error : {2}", 
                this->ActivityId,
                composeFilePath_,
                error);

            TryComplete(thisSPtr, error);
            return;
        }
    }

    error = ReadFileContents(composeFile, overridesFile);
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    auto asyncOperation = Client.BeginInnerCreateComposeDeployment(
        deploymentName_,
        move(this->composeFileContents_),
        move(this->overridesFileContents_),
        this->repositoryUserName_,
        this->repositoryPassword_,
        this->isPasswordEncrypted_,
        this->ActivityId,
        this->RemainingTime,
        [this](AsyncOperationSPtr const operation)
        {
            this->OnCreateContainerAppComplete(operation, false);
        },
        thisSPtr);

    OnCreateContainerAppComplete(asyncOperation, true);
}

void FabricClientImpl::CreateContainerAppAsyncOperation::OnCreateContainerAppComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously)
    {
        return;
    }

    auto error = this->Client.EndInnerCreateComposeDeployment(operation);
    TryComplete(operation->Parent, error);
}

ErrorCode FabricClientImpl::CreateContainerAppAsyncOperation::ReadFileContents(File &composeFile, File &overridesFile)
{
    size_t composeFileSize = composeFile.size();
    size_t overridesFileSize = 0;
    if (overridesFile.IsValid())
    {
        overridesFileSize = overridesFile.size();
    }

    size_t totalSize = 0;
    if (!SUCCEEDED(SizeTAdd(composeFileSize, overridesFileSize, &totalSize)) ||
        (totalSize > INT_MAX) ||
        (totalSize > Utility::GetMessageContentThreshold()))
    {
        WriteInfo(
            TraceComponent, 
            "{0}: Total file sizes exceed the max message size threshold - ComposeFileSize - {1}, OverridesFileSize - {2}, Threshold - {3}",
            this->ActivityId,
            composeFileSize,
            overridesFileSize,
            Utility::GetMessageContentThreshold());

        return ErrorCodeValue::MessageTooLarge;
    }

    DWORD bytesRead;
    int size = static_cast<int>(composeFileSize);
    composeFileContents_ = vector<BYTE>(size);
    auto error = composeFile.TryRead2(composeFileContents_.data(), size, bytesRead);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0}: ReadFile failed - filename : '{1},' error : {2}",
            this->ActivityId,
            composeFilePath_,
            error);

        return error;
    }

    if (overridesFile.IsValid())
    {
        size = static_cast<int>(overridesFileSize);
        overridesFileContents_ = vector<BYTE>(static_cast<int>(size));
        error = overridesFile.TryRead2(overridesFileContents_.data(), size, bytesRead);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0}: ReadFile failed - filename : '{1},' error : {2}",
                this->ActivityId,
                overridesFilePath_,
                error);

            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FabricClientImpl::CreateContainerAppAsyncOperation::End(AsyncOperationSPtr const &operation)
{
    auto casted = AsyncOperation::End<CreateContainerAppAsyncOperation>(operation);
    return casted->Error;
}

