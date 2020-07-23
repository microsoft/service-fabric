// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
// ********************************************************************************************************************

//SystemServiceOperation
template<class TComInterface>
class Api::ComProxySystemServiceBase<TComInterface>::CallSystemServiceOperation : public Common::ComProxyAsyncOperation
{
    DENY_COPY(CallSystemServiceOperation)

public:
    CallSystemServiceOperation(
        __in TComInterface & comImpl,
        __in LPCWSTR action,
        __in LPCWSTR inputBlob,
		TimeSpan const & timeout,
		AsyncCallback const & callback,
		AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , action_(action)
        , inputBlob_(inputBlob)
        , timeout_(timeout)
    {
    }

    virtual ~CallSystemServiceOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricStringResult ** reply)
    {
        auto casted = AsyncOperation::End<CallSystemServiceOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result_);
        }

        return casted->Error;
    }

protected:

    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginCallSystemService(
            action_,
            inputBlob_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndCallSystemService(context, (IFabricStringResult**)&result_);
    }

	TComInterface & comImpl_;
    LPCWSTR inputBlob_;
    LPCWSTR action_;
    IFabricStringResult * result_;
	TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxySystemServiceBase Implementation
//

template<class TComInterface>
Api::ComProxySystemServiceBase<TComInterface>::ComProxySystemServiceBase(
	ComPointer<TComInterface> const & comImpl)
	: comImpl_(comImpl)
{
}

template<class TComInterface>
Api::ComProxySystemServiceBase<TComInterface>::~ComProxySystemServiceBase()
{
}

// SystemServiceCall
template<class TComInterface>
AsyncOperationSPtr Api::ComProxySystemServiceBase<TComInterface>::BeginCallSystemServiceInternal(
    wstring const & action,
    wstring const & inputBlob,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CallSystemServiceOperation>(
        *comImpl_.GetRawPointer(),
        action.c_str(),
        inputBlob.c_str(),
        timeout,
        callback,
        parent);
}

template<class TComInterface>
ErrorCode Api::ComProxySystemServiceBase<TComInterface>::EndCallSystemServiceInternal(
    AsyncOperationSPtr const & asyncOperation,
    __inout wstring & outputBlob)
{
    ComPointer<IFabricStringResult> publicOutput;
    ErrorCode result = CallSystemServiceOperation::End(asyncOperation, publicOutput.InitializationAddress());

    if (result.IsSuccess())
    {
        result = StringUtility::LpcwstrToWstring2(
            publicOutput->get_String(),
            true,
            0,
            STRSAFE_MAX_CCH,
            outputBlob
        );
    }

    return result;
}

