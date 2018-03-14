// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ComConfigPackage::ComConfigPackage(
    ConfigPackageDescription const & configPackageDescription, 
    wstring const& serviceManifestName,
    wstring const& serviceManifestVersion,
    wstring const & path,
    ConfigSettings const & settings) 
    : heap_(),
    configPackageDescription_(),
    path_(path),
    configSettings_()
{
    configPackageDescription_ = heap_.AddItem<FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION>();
    configPackageDescription.ToPublicApi(heap_, serviceManifestName, serviceManifestVersion, *configPackageDescription_);

    configSettings_ = heap_.AddItem<FABRIC_CONFIGURATION_SETTINGS>();
    settings.ToPublicApi(heap_, *configSettings_);
}

ComConfigPackage::~ComConfigPackage()
{
}

const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * ComConfigPackage::get_Description(void)
{
    return configPackageDescription_.GetRawPointer();
}

LPCWSTR ComConfigPackage::get_Path()
{
    return path_.c_str();
}

const FABRIC_CONFIGURATION_SETTINGS * ComConfigPackage::get_Settings(void)
{
    return configSettings_.GetRawPointer();
}

HRESULT ComConfigPackage::GetSection( 
    /* [in] */ LPCWSTR sectionName,
    /* [retval][out] */ const FABRIC_CONFIGURATION_SECTION **bufferedValue)
{
    if ((sectionName == NULL) || (bufferedValue == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return FindSection(configSettings_.GetRawPointer(), sectionName, bufferedValue);
}
        
HRESULT ComConfigPackage::GetValue( 
    /* [in] */ LPCWSTR sectionName,
    /* [in] */ LPCWSTR parameterName,
    /* [out] */ BOOLEAN *isEncrypted,
    /* [retval][out] */ LPCWSTR *bufferedValue)
{
    if ((sectionName == NULL) || (parameterName == NULL) || (bufferedValue == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    const FABRIC_CONFIGURATION_SECTION * section;
    auto hr = FindSection(configSettings_.GetRawPointer(), sectionName, &section);
    if (FAILED(hr))
    {
        return hr;
    }

    const FABRIC_CONFIGURATION_PARAMETER * parameter;
    hr = FindParameter(section, parameterName, &parameter);
    if (FAILED(hr))
    {
        return hr;
    }
    
    *isEncrypted = parameter->IsEncrypted;
    *bufferedValue = parameter->Value;    

    return S_OK;
}

HRESULT ComConfigPackage::GetValues(
    /* [in] */ LPCWSTR sectionName,
    /* [in] */ LPCWSTR parameterPrefix,
    /* [out, retval] */ FABRIC_CONFIGURATION_PARAMETER_LIST ** bufferedValue)
{
    if ((sectionName == NULL) || (parameterPrefix == NULL) || (bufferedValue == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    const FABRIC_CONFIGURATION_SECTION * section;
    auto hr = FindSection(configSettings_.GetRawPointer(), sectionName, &section);
    if (FAILED(hr))
    {
        return hr;
    }

    std::list<FABRIC_CONFIGURATION_PARAMETER> values;
 
    for (size_t ix = 0; ix < section->Parameters->Count; ++ix)
    {
        if (StringUtility::StartsWithCaseInsensitive<wstring>(section->Parameters->Items[ix].Name, parameterPrefix))
        {
            values.emplace_back(section->Parameters->Items[ix]);
        }
    }

    if (values.empty())
    {
        return ::FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND;
    }

    auto parameterList = heap_.AddItem<FABRIC_CONFIGURATION_PARAMETER_LIST>();
    parameterList->Count = static_cast<ULONG>(values.size());
    auto parameterItems = heap_.AddArray<FABRIC_CONFIGURATION_PARAMETER>(values.size());
    parameterList->Items = parameterItems.GetRawArray();
    int i = 0;
    for (auto iter = values.begin(); iter != values.end(); ++iter)
    {
        parameterItems[i] = *iter;
        i++;
    }

    *bufferedValue = parameterList.GetRawPointer();

    return S_OK;
}

HRESULT ComConfigPackage::DecryptValue(
    /* [in] */ LPCWSTR encryptedValue,
    /* [out, retval] */ IFabricStringResult ** decryptedValue)
{
    if(encryptedValue == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    SecureString result;
    ErrorCode error = CryptoUtility::DecryptText(std::wstring(encryptedValue), result);

    if(error.IsSuccess())
    {
        return ComSecureStringResult::ReturnStringResult(result, decryptedValue);
    }
    else
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }
}

HRESULT ComConfigPackage::FindSection(
    const FABRIC_CONFIGURATION_SETTINGS * settings,
    LPCWSTR sectionName,
    const FABRIC_CONFIGURATION_SECTION **section)
{
    for(size_t ix = 0; ix < settings->Sections->Count; ++ix)
    {
        if (StringUtility::AreEqualCaseInsensitive(sectionName, settings->Sections->Items[ix].Name))
        {
            *section = &settings->Sections->Items[ix];
            return S_OK;
        }
    }

    return ::FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND;
}

HRESULT ComConfigPackage::FindParameter(
    const FABRIC_CONFIGURATION_SECTION * section,
    LPCWSTR parameterName,
    const FABRIC_CONFIGURATION_PARAMETER **parameter)
{
    for(size_t ix = 0; ix < section->Parameters->Count; ++ix)
    {
        if (StringUtility::AreEqualCaseInsensitive(parameterName, section->Parameters->Items[ix].Name))
        {
            *parameter = &section->Parameters->Items[ix];
            return S_OK;
        }
    }

    return ::FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND;
}

