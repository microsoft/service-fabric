// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace FileStoreService;

StringLiteral const TraceComponent("ImpersonatedSMBCopyContext");   

ImpersonatedSMBCopyContext::ImpersonatedSMBCopyContext(
    AccessTokensCollection && accessTokens,
    GetAccessTokensCallback getAccessTokensCallback)
    : accessTokens_(move(accessTokens))
    , getAccessTokensCallback_(getAccessTokensCallback)
    , lastTokenRefreshTime_(Stopwatch::Now())
    , lock_()
    , traceId_()
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));
}

ErrorCode ImpersonatedSMBCopyContext::Create(__inout ImpersonatedSMBCopyContextSPtr & smbCopyContext)
{
    GetAccessTokensCallback failureCallback = NULL;

    AccessTokensCollection tokenMap;
    AccessTokensList tokens;
    auto error = Utility::GetAccessTokens(tokenMap, tokens);
    bool setFailureCallback = false;
    if (!error.IsSuccess())
    {
        if (tokens.empty())
        {
            // No primary, secondary or common names tokens are found
            return error;
        }

        setFailureCallback = true;
    }
    else if (Utility::HasCommonNameAccountsConfigured())
    {
        // Always set the callback for common names to be able to load new accounts from added certificates
        setFailureCallback = true;
    }

    if (setFailureCallback)
    {
        failureCallback = [](__inout AccessTokensCollection & tokenMap, __inout AccessTokensList & tokens)
        {
            return Utility::GetAccessTokens(tokenMap, tokens);
        };
    }

    if (FileStoreServiceConfig::GetConfig().AnonymousAccessEnabled || tokenMap.empty())
    {
        // Adding empty AccessToken to access share as current process user
        tokenMap.insert(make_pair(L"CurrentUser", AccessTokenSPtr()));
    }

    smbCopyContext = make_shared<ImpersonatedSMBCopyContext>(move(tokenMap), failureCallback);

    return ErrorCodeValue::Success;
}

ErrorCode ImpersonatedSMBCopyContext::CopyFile(std::wstring const & sourcePath, std::wstring const & destinationPath)
{
    StopwatchTime lastRefreshTime;

    // Get the list of current tokens under the read lock
    AccessTokensList currentTokens;
    {
        AcquireReadLock lock(this->lock_);
        lastRefreshTime = lastTokenRefreshTime_;
        for (auto const & entry : accessTokens_)
        {
            currentTokens.push_back(entry.second);
        }
    }

    size_t currentTokenCount = currentTokens.size();

    // Try the current tokens
    auto error = ImpersonateAndCopyFile(sourcePath, destinationPath, currentTokens);
    if (error.IsSuccess())
    {
        return error;
    }

    auto lastError = error;

    // Failure callback is never reset, so it's safe to access outside lock
    AccessTokensList newTokens;
    if (this->getAccessTokensCallback_ != NULL)
    {
        AcquireWriteLock lock(this->lock_);
        if (lastTokenRefreshTime_ == lastRefreshTime)
        {
            // No other thread refreshed the tokens since this thread got the results.
            lastTokenRefreshTime_ = Stopwatch::Now();
            auto accessTokenErrorCode = this->getAccessTokensCallback_(accessTokens_, newTokens);
            WriteTrace(
                accessTokenErrorCode.ToLogLevel(),
                TraceComponent,
                traceId_,
                "CopyFile: get new AccessTokens completed with {0}, {1} new tokens, refresh time {2}.",
                accessTokenErrorCode,
                newTokens.size(),
                lastRefreshTime);
            if (!accessTokenErrorCode.IsSuccess() || newTokens.empty())
            {
                // No new tokens found
                return error;
            }
        }
        else
        {
            for (auto const & entry : accessTokens_)
            {
                if (find(currentTokens.begin(), currentTokens.end(), entry.second) == currentTokens.end())
                {
                    newTokens.push_back(entry.second);
                }
            }
        }
    }

    if (newTokens.empty())
    {
        WriteWarning(
            TraceComponent,
            traceId_,
            "CopyFile: no new token is found. current token count: {0}", currentTokenCount);
        return error;
    }

    // Try the new tokens
    error = ImpersonateAndCopyFile(sourcePath, destinationPath, newTokens);
    if (error.IsSuccess())
    {
        return error;
    }
    else
    {
        if (!ImpersonatedSMBCopyContext::IsRetryableError(error))
        {
            return ImpersonatedSMBCopyContext::IsRetryableError(lastError) ? lastError : error;
        }
        else
        {
            return error;
        }
    }
}

ErrorCode ImpersonatedSMBCopyContext::ImpersonateAndCopyFile(
    wstring const & sourcePath,
    wstring const & destinationPath,
    AccessTokensList const & accessTokens)
{
    ASSERT_IF(accessTokens.empty(), "ImpersonateAndCopyFile: no tokens to impersonate");
    ErrorCode lastError(ErrorCodeValue::Success);
    for (auto const & token : accessTokens)
    {
        auto error = this->ImpersonateAndCopyFile(sourcePath, destinationPath, token);
        if (error.IsSuccess())
        {
            return error;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "ImpersonateAndCopyFile for SourcePath:{0}, DestinationPath:{1} failed: {2}.",
                sourcePath,
                destinationPath,
                error);

            // Update lastError only in case of non-retryable error.
            // Because non-retryable error should not overwrite retryable error resulting in retry not done.
            // This can happen when earlier token returns retryable error code (ERROR_SHARING_VIOLATION) and 
            // subsequent copy with diff. token can result in non-retryable error (ACCESS_DENIED).
            if (!ImpersonatedSMBCopyContext::IsRetryableError(lastError) || lastError.IsSuccess())
            {
                lastError.Overwrite(error);
            }
        }
    }

    WriteWarning(
        TraceComponent,
        traceId_,
        "ImpersonateAndCopyFile for SourcePath:{0}, DestinationPath:{1} failed: {2}. Have tried all access tokens.",
        sourcePath,
        destinationPath,
        lastError);

    return lastError;
}

ErrorCode ImpersonatedSMBCopyContext::ImpersonateAndCopyFile(
    wstring const & sourcePath,
    wstring const & destinationPath,
    AccessTokenSPtr const & accessToken)
{
    ImpersonationContextUPtr impersonationContext;

#if !defined(PLATFORM_UNIX)
    if(accessToken)
    {
        auto error = accessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "Failed to impersonate with {0}. SourcePath:{1}, DestPath:{2}",
                error,
                sourcePath,
                destinationPath);
            return error;
        }
    }
#else
    string accountName, password;
    if(accessToken != nullptr && !accessToken->AccountName.empty())
        StringUtility::Utf16ToUtf8(accessToken->AccountName, accountName);
    if(accessToken!= nullptr && !accessToken->Password.empty())
        StringUtility::Utf16ToUtf8(accessToken->Password, password);
    if(accountName.empty() || password.empty())
    {
        return ErrorCodeValue::AccessDenied;
    }

    wstring directory = Path::GetDirectoryName(destinationPath);

    if(!Directory::Exists(directory) && directory.find(L":/")==wstring::npos)
    {
        auto error = Directory::Create2(directory);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                traceId_,
                "Failed to create directory. Path:{0}, Error:{1}",
                directory,
                error);
        }
    }
#endif

    Stopwatch watch;
    watch.Start();

#if !defined(PLATFORM_UNIX)
    auto error = File::SafeCopy(sourcePath, destinationPath, true /*overwrite*/, false /*shouldAcquireLock*/);
#else
    auto error = File::Copy(sourcePath, destinationPath, accessToken->AccountName, accessToken->Password, true /*overwrite*/);
#endif
    watch.Stop();
    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
        TraceComponent,
        traceId_,
        "CopyFile: SourcePath:{0}, DestinationPath:{1}, Error:{2}, ElapsedTime:{3}",
        sourcePath,
        destinationPath,
        error,
        watch.ElapsedMilliseconds);

    return error;
}

bool ImpersonatedSMBCopyContext::IsRetryableError(ErrorCode const & error)
{
    if (IsRetryableNetworkError(error) ||
        error.IsWin32Error(ERROR_PATH_NOT_FOUND) ||
        error.IsWin32Error(ERROR_SHARING_VIOLATION))
    {
        return true;
    }

    return false;
}

bool ImpersonatedSMBCopyContext::IsRetryableNetworkError(ErrorCode const & error)
{
    if (error.IsWin32Error(ERROR_BAD_NETPATH) ||
        error.IsWin32Error(ERROR_BAD_NET_NAME) ||
        error.IsWin32Error(ERROR_NETNAME_DELETED))
    {
        return true;
    }

    return false;
}
