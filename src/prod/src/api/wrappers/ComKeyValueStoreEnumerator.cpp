// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComKeyValueStoreEnumerator::ComKeyValueStoreEnumerator Implementation
//

ComKeyValueStoreEnumerator::ComKeyValueStoreEnumerator(IStoreEnumeratorPtr const & impl)
    : IFabricKeyValueStoreEnumerator2(),
    impl_(impl)
{
}

ComKeyValueStoreEnumerator::~ComKeyValueStoreEnumerator()
{
}

HRESULT ComKeyValueStoreEnumerator::EnumerateByKey( 
    /* [in] */ LPCWSTR keyPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result)
{
    if (result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, true /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    IStoreItemEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateKeyValueStore(
        keyPrefixStr,
        true, // strictPrefix
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreEnumerator::EnumerateMetadataByKey( 
    /* [in] */ LPCWSTR keyPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result)
{
    if (result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, true /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    IStoreItemMetadataEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateKeyValueStoreMetadata(
        keyPrefixStr,
        true, // strictPrefix
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}

//
// IFabricKeyValueStoreEnumerator2 methods
// 

HRESULT ComKeyValueStoreEnumerator::EnumerateByKey2( 
    /* [in] */ LPCWSTR keyPrefix,
    /* [in] */ BOOLEAN strictPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result)
{
    if (result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, true /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    IStoreItemEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateKeyValueStore(
        keyPrefixStr,
        (strictPrefix != FALSE),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
        
HRESULT ComKeyValueStoreEnumerator::EnumerateMetadataByKey2( 
    /* [in] */ LPCWSTR keyPrefix,
    /* [in] */ BOOLEAN strictPrefix,
    /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result)
{
    if (result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    wstring keyPrefixStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(keyPrefix, true /* acceptsNull */, keyPrefixStr);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr);
    }

    IStoreItemMetadataEnumeratorPtr resultImpl;
    auto error = impl_->EnumerateKeyValueStoreMetadata(
        keyPrefixStr,
        (strictPrefix != FALSE),
        resultImpl);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}
