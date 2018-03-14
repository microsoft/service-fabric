// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace ServiceModel;

class TestTokenAuthenticator::AuthenticateAsyncOperation : public AsyncOperation
{
    DENY_COPY(AuthenticateAsyncOperation);
public:

    AuthenticateAsyncOperation(
        wstring const& inputToken,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , inputToken_(inputToken)
        , timeout_(timeout)
        , expirationTime_(TimeSpan::MaxValue)
    {
    }

    static ErrorCode End(
        Common::AsyncOperationSPtr const & operationSPtr,
        __out Common::TimeSpan &expirationTime,
        __out std::vector<Claim> &claimsList)
    {
        AuthenticateAsyncOperation* thisPtr = AsyncOperation::End<AuthenticateAsyncOperation>(operationSPtr);
        if (thisPtr->Error.IsSuccess())
        {
            expirationTime = move(thisPtr->expirationTime_);
            claimsList = move(thisPtr->claimsList_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        // split "Claim && Claim && Claim ..."
        vector<wstring> claimVector;
        StringUtility::Split<wstring>(inputToken_, claimVector, L"&&", true);
        if (!claimVector.empty())
        {
            for (wstring const& claim : claimVector)
            {
                Claim c;
                // split "ClaimType=ClaimValue"
                vector<wstring> claimParts;
                StringUtility::Split<wstring>(claim, claimParts, L"=", true);
                if (claimParts.size() != 2)
                {
                    TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
                    return;
                }
                c.ClaimType = claimParts[0];
                c.Value = claimParts[1];
                claimsList_.push_back(move(c));
            }
        }

        TryComplete(thisSPtr, ErrorCode::Success());
    }

private:
    wstring inputToken_;
    TimeSpan timeout_;
    vector<Claim> claimsList_;
    TimeSpan expirationTime_;
};

AsyncOperationSPtr TestTokenAuthenticator::BeginAuthenticate(
    wstring const& inputToken,
    TimeSpan const timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<AuthenticateAsyncOperation>(
        inputToken,
        timeout,
        callback,
        parent);
}

ErrorCode TestTokenAuthenticator::EndAuthenticate(
    Common::AsyncOperationSPtr const& operation,
    __out Common::TimeSpan &expirationTime,
    __out std::vector<Claim> &claimsList)
{
    return AuthenticateAsyncOperation::End(operation, expirationTime, claimsList);
}
