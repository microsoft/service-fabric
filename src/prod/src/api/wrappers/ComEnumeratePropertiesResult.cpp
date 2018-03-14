// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;

namespace Api
{
    ComEnumeratePropertiesResult::ComEnumeratePropertiesResult(
        EnumeratePropertiesResult && enumerationResult)
        : enumerationResult_(std::move(enumerationResult))
        , comProperties_()
    {
        for (auto iter = enumerationResult_.Properties.begin(); iter != enumerationResult_.Properties.end(); ++iter)
        {
            NamePropertyResult & propertyResult = const_cast<NamePropertyResult &>(*iter);
            auto comProperty = make_com<ComNamedProperty>(std::move(propertyResult));

            comProperties_.push_back(move(comProperty));
        }
    }

    EnumeratePropertiesToken const & ComEnumeratePropertiesResult::get_ContinuationToken()
    {
        return enumerationResult_.ContinuationToken;
    }

    FABRIC_ENUMERATION_STATUS STDMETHODCALLTYPE ComEnumeratePropertiesResult::get_EnumerationStatus()
    {
        return enumerationResult_.FABRIC_ENUMERATION_STATUS;
    }

    ULONG STDMETHODCALLTYPE ComEnumeratePropertiesResult::get_PropertyCount()
    {
        return static_cast<ULONG>(comProperties_.size());
    }

    HRESULT STDMETHODCALLTYPE ComEnumeratePropertiesResult::GetProperty(
        ULONG index,
        __out IFabricPropertyValueResult ** property)
    {
        if (property == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        if (static_cast<size_t>(index) >= comProperties_.size())
        {
            return ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }

        ComNamedPropertyCPtr resultCPtr = comProperties_[index];
        *property = resultCPtr.DetachNoRelease();

        return ComUtility::OnPublicApiReturn(S_OK);
    }
}
