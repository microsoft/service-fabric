// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common 
{
    using namespace std;
    
    /******************** ComStringResult implementation ****************************/
    ComStringResult::ComStringResult(wstring const & value)
        : value_(value)
    {
    }
    
    ComStringResult::~ComStringResult()
    {
    }

    LPCWSTR STDMETHODCALLTYPE ComStringResult::get_String(void)
    {
        return this->value_.c_str();
    }

    HRESULT ComStringResult::ReturnStringResult(std::wstring const & result, IFabricStringResult ** value)
    {
        if (value == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(result);
        *value = stringResult.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
    
    /******************** ComSecureStringResult implementation ****************************/
        ComSecureStringResult::ComSecureStringResult(SecureString const & value)
        : value_(value)
    {
    }
    
    ComSecureStringResult::~ComSecureStringResult()
    {
    }

    LPCWSTR STDMETHODCALLTYPE ComSecureStringResult::get_String(void)
    {
        return this->value_.GetPlaintext().c_str();
    }

    HRESULT ComSecureStringResult::ReturnStringResult(SecureString const & result, IFabricStringResult ** value)
    {
        if (value == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<IFabricStringResult> stringResult = make_com<ComSecureStringResult, IFabricStringResult>(result);
        *value = stringResult.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);        
    }

    /******************** ComStringCollectionResult implementation ****************************/
    ComStringCollectionResult::ComStringCollectionResult(StringCollection && collection)
        : ComUnknownBase(),
        heap_(),
        stringCollection_()
    {
        auto count = collection.size();
        if (count > 0)
        {
            stringCollection_ = heap_.AddArray<LPCWSTR>(count);
            for (size_t i = 0; i < count; ++i)
            {
                stringCollection_[i] = heap_.AddString(collection[i]);
            }
        }
    }

    ComStringCollectionResult::~ComStringCollectionResult()
    {
    }

    HRESULT STDMETHODCALLTYPE ComStringCollectionResult::GetStrings( 
        /* [out] */ ULONG *itemCount,
        /* [retval][out] */ const LPCWSTR **bufferedItems)
    {
        if ((itemCount == NULL) || (bufferedItems == NULL))
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        *itemCount = static_cast<ULONG>(stringCollection_.GetCount());
        if(*itemCount > 0)
        {
            *bufferedItems = stringCollection_.GetRawArray();
        }
        else
        {
            *bufferedItems = NULL;
        }

        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT ComStringCollectionResult::ReturnStringCollectionResult(StringCollection && result, IFabricStringListResult ** value)
    {
        ComPointer<IFabricStringListResult> stringCollectionResult = make_com<ComStringCollectionResult, IFabricStringListResult>(move(result));
        *value = stringCollectionResult.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
} // end namespace Common
