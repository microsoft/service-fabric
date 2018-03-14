// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// LogCollectionProviderFactory::DummyLogCollectionProvider Implementation
//
class LogCollectionProviderFactory::DummyLogCollectionProvider : public ILogCollectionProvider
{
public:
    DummyLogCollectionProvider(ComponentRoot const & root, wstring const & nodePath)
        : ILogCollectionProvider(root, nodePath)
    {
    }

    AsyncOperationSPtr BeginAddLogPaths(
        wstring const & applicationId,
        vector<wstring> const & paths,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationId);
        UNREFERENCED_PARAMETER(paths);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::Success),
            callback, 
            parent);
    }

    ErrorCode EndAddLogPaths(AsyncOperationSPtr const & asyncOperation)
    {
        return CompletedAsyncOperation::End(asyncOperation);
    }

    // Remove log collection from the specified folder 
    // and all inner folders.
    AsyncOperationSPtr BeginRemoveLogPaths(
        wstring const & path,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(path);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::Success),
            callback, 
            parent);
    }

    ErrorCode EndRemoveLogPaths(AsyncOperationSPtr const & asyncOperation)
    {
        return CompletedAsyncOperation::End(asyncOperation);
    }

    void CleanupLogPaths(wstring const & path)
    {
        UNREFERENCED_PARAMETER(path);
    }

    AsyncOperationSPtr OnBeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::Success),
            callback, 
            parent);
    }

    ErrorCode OnEndOpen(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr OnBeginClose(
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::Success),
            callback, 
            parent);
    }

    ErrorCode OnEndClose(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    void OnAbort()
    {
        // Nothing to do
    }
};

ErrorCode LogCollectionProviderFactory::CreateLogCollectionProvider(
    ComponentRoot const & root,
    wstring const &,
    wstring const & nodePath,
    __out unique_ptr<ILogCollectionProvider> & provider)
{
    // TODO: create correct provider based on the infrastructure
    provider = make_unique<DummyLogCollectionProvider>(root, nodePath);

    return ErrorCode(ErrorCodeValue::Success);
}
