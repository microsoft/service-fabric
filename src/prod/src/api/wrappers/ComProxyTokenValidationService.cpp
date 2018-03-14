// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyTokenValidationService::ValidateTokenAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ValidateTokenAsyncOperation)

public:
    ValidateTokenAsyncOperation(
        __in IFabricTokenValidationService & comImpl,
        wstring const & authToken,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , authToken_(authToken)
        , timeout_(timeout)
    {
    }

    virtual ~ValidateTokenAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricTokenClaimResult ** claims)
    {
        auto thisPtr = AsyncOperation::End<ValidateTokenAsyncOperation>(operation);
        if(thisPtr->Error.IsSuccess())
        {
            *claims = move(thisPtr->result);
        }
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginValidateToken(
            authToken_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndValidateToken(context, (void**)&result);
    }
     
    IFabricTokenValidationService & comImpl_;
    wstring authToken_;
    TimeSpan const timeout_;
    IFabricTokenClaimResult* result;
};


// ********************************************************************************************************************
// ComProxyTokenValidationService Implementation
//
ComProxyTokenValidationService::ComProxyTokenValidationService(
    ComPointer<IFabricTokenValidationService> const & comImpl)
    : ITokenValidationService()
    , comImpl_(comImpl)
{
}

ComProxyTokenValidationService::~ComProxyTokenValidationService()
{
}

AsyncOperationSPtr ComProxyTokenValidationService::BeginValidateToken(
    wstring const & authToken,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ValidateTokenAsyncOperation>(
        *comImpl_.GetRawPointer(),
        authToken,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyTokenValidationService::EndValidateToken(
    AsyncOperationSPtr const & asyncOperation,
    IFabricTokenClaimResult** result)
{
    return ValidateTokenAsyncOperation::End(asyncOperation, result);
}

ErrorCode ComProxyTokenValidationService::GetTokenServiceMetadata(IFabricTokenServiceMetadataResult** result)
{
    HRESULT err = comImpl_->GetTokenServiceMetadata(result);
    return ErrorCode::FromHResult(err);
}
