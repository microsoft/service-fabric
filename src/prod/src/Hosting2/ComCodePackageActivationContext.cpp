// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType("ComCodePackageActivationContext");

ComCodePackageActivationContext::ComCodePackageActivationContext(CodePackageActivationContextSPtr const& codePackageActivationContextSPtr)
    : codePackageActivationContextSPtr_(codePackageActivationContextSPtr)
{
    id_ = Guid::NewGuid().ToString();
    codePackageActivationContextSPtr_->RegisterITentativeCodePackageActivationContext(id_, *this);
}

ComCodePackageActivationContext::~ComCodePackageActivationContext()
{
    codePackageActivationContextSPtr_->UnregisterITentativeCodePackageActivationContext(id_);
}

ULONG STDMETHODCALLTYPE ComCodePackageActivationContext::TryAddRef()
{
    return this->BaseTryAddRef();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_ContextId(void)
{
    return codePackageActivationContextSPtr_->ContextId.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_CodePackageName(void)
{
    return codePackageActivationContextSPtr_->CodePackageName.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_CodePackageVersion(void)
{
    return codePackageActivationContextSPtr_->CodePackageVersion.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_WorkDirectory(void)
{
    return codePackageActivationContextSPtr_->WorkDirectory.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_LogDirectory(void)
{
    return codePackageActivationContextSPtr_->LogDirectory.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_TempDirectory(void)
{
    return codePackageActivationContextSPtr_->TempDirectory.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_ServiceListenAddress(void)
{
    return codePackageActivationContextSPtr_->ServiceListenAddress.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_ServicePublishAddress(void)
{
    return codePackageActivationContextSPtr_->ServicePublishAddress.c_str();
}

const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * STDMETHODCALLTYPE ComCodePackageActivationContext::get_ServiceEndpointResources(void)
{
    return codePackageActivationContextSPtr_->EndpointResourceList.GetRawPointer();
}

const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST * STDMETHODCALLTYPE ComCodePackageActivationContext::get_ServiceTypes(void)
{
    return codePackageActivationContextSPtr_->ServiceTypesList.GetRawPointer();
}

const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST * STDMETHODCALLTYPE ComCodePackageActivationContext::get_ServiceGroupTypes(void)
{
    return codePackageActivationContextSPtr_->ServiceGroupTypesList.GetRawPointer();
}

const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION * STDMETHODCALLTYPE ComCodePackageActivationContext::get_ApplicationPrincipals(void)
{
    return codePackageActivationContextSPtr_->ApplicationPrincipals.GetRawPointer();
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetCodePackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    StringCollection nameCollection;
    codePackageActivationContextSPtr_->GetCodePackageNames(nameCollection);
    return ComStringCollectionResult::ReturnStringCollectionResult(move(nameCollection), names);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetDataPackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    StringCollection nameCollection;
    codePackageActivationContextSPtr_->GetDataPackageNames(nameCollection);
    return ComStringCollectionResult::ReturnStringCollectionResult(move(nameCollection), names);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetConfigurationPackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    StringCollection nameCollection;
    codePackageActivationContextSPtr_->GetConfigurationPackageNames(nameCollection);
    return ComStringCollectionResult::ReturnStringCollectionResult(move(nameCollection), names);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetCodePackage(
    /* [in] */ LPCWSTR codePackageName,
    /* [retval][out] */ IFabricCodePackage **codePackage)
{
    if (codePackageName == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (codePackage == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring codePackageNameString(codePackageName);
    ComPointer<IFabricCodePackage> comCodePackage;
    ErrorCode error = codePackageActivationContextSPtr_->GetCodePackage(codePackageNameString, comCodePackage);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
    return ComUtility::OnPublicApiReturn(comCodePackage->QueryInterface(IID_IFabricCodePackage, reinterpret_cast<void**>(codePackage)));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetConfigurationPackage(
    /* [in] */ LPCWSTR configPackageName,
    /* [retval][out] */ IFabricConfigurationPackage **configPackage)
{
    if (configPackageName == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (configPackage == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring configPackageNameString(configPackageName);
    ComPointer<IFabricConfigurationPackage> comConfigPackage;
    ErrorCode error = codePackageActivationContextSPtr_->GetConfigurationPackage(configPackageNameString, comConfigPackage);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
    return ComUtility::OnPublicApiReturn(comConfigPackage->QueryInterface(IID_IFabricConfigurationPackage, reinterpret_cast<void**>(configPackage)));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetDataPackage(
    /* [in] */ LPCWSTR dataPackageName,
    /* [retval][out] */ IFabricDataPackage **dataPackage)
{
    if (dataPackageName == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (dataPackage == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring dataPackageNameString(dataPackageName);
    ComPointer<IFabricDataPackage> comDataPackage;
    ErrorCode error = codePackageActivationContextSPtr_->GetDataPackage(dataPackageNameString, comDataPackage);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
    return ComUtility::OnPublicApiReturn(comDataPackage->QueryInterface(IID_IFabricDataPackage, reinterpret_cast<void**>(dataPackage)));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetServiceEndpointResource(
    /*[in]*/ LPCWSTR serviceEndpointResourceName,
    /*[out, retval]*/ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION ** bufferedValue)
{
    if (serviceEndpointResourceName == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (bufferedValue == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring endpointResourceNameString(serviceEndpointResourceName);

    for (ULONG i = 0; i < codePackageActivationContextSPtr_->EndpointResourceList->Count; ++i)
    {
        if (StringUtility::AreEqualCaseInsensitive(endpointResourceNameString, wstring(codePackageActivationContextSPtr_->EndpointResourceList->Items[i].Name)))
        {
            *bufferedValue = &(codePackageActivationContextSPtr_->EndpointResourceList->Items[i]);
            return S_OK;
        }
    }

    return FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND;
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::RegisterCodePackageChangeHandler(
    /* [in] */ IFabricCodePackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    if (callback == NULL || callbackHandle == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricCodePackageChangeHandler> handlerCPtr;
    handlerCPtr.SetAndAddRef(callback);

    LONGLONG handleId = codePackageActivationContextSPtr_->RegisterCodePackageChangeHandler(
        handlerCPtr,
        id_);

    *callbackHandle = handleId;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::UnregisterCodePackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    codePackageActivationContextSPtr_->UnregisterCodePackageChangeHandler(callbackHandle);

    return S_OK;
}
HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::RegisterConfigurationPackageChangeHandler(
    /* [in] */ IFabricConfigurationPackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    if (callback == NULL || callbackHandle == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricConfigurationPackageChangeHandler> handlerCPtr;
    handlerCPtr.SetAndAddRef(callback);

    LONGLONG handleId = codePackageActivationContextSPtr_->RegisterConfigurationPackageChangeHandler(
        handlerCPtr,
        id_);

    *callbackHandle = handleId;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::UnregisterConfigurationPackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    codePackageActivationContextSPtr_->UnregisterConfigurationPackageChangeHandler(callbackHandle);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::RegisterDataPackageChangeHandler(
    /* [in] */ IFabricDataPackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    if (callback == NULL || callbackHandle == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricDataPackageChangeHandler> handlerCPtr;
    handlerCPtr.SetAndAddRef(callback);

    LONGLONG handleId = codePackageActivationContextSPtr_->RegisterDataPackageChangeHandler(
        handlerCPtr,
        id_);

    *callbackHandle = handleId;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::UnregisterDataPackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    codePackageActivationContextSPtr_->UnregisterDataPackageChangeHandler(callbackHandle);

    return S_OK;
}

FABRIC_URI STDMETHODCALLTYPE ComCodePackageActivationContext::get_ApplicationName(void)
{
    return codePackageActivationContextSPtr_->ApplicationName.c_str();
}

LPCWSTR STDMETHODCALLTYPE ComCodePackageActivationContext::get_ApplicationTypeName(void)
{
    return codePackageActivationContextSPtr_->ApplicationTypeName.c_str();
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetServiceManifestName(
    /* [retval][out] */ IFabricStringResult **serviceManifestName)
{
    wstring result;
    codePackageActivationContextSPtr_->GetServiceManifestName(result);
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(result, serviceManifestName));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetServiceManifestVersion(
    /* [retval][out] */ IFabricStringResult **serviceManifestVersion)
{
    wstring result;
    codePackageActivationContextSPtr_->GetServiceManifestVersion(result);
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(result, serviceManifestVersion));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportApplicationHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportApplicationHealth2(healthInformation, nullptr);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportDeployedApplicationHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportDeployedApplicationHealth2(healthInformation, nullptr);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportDeployedServicePackageHealth(FABRIC_HEALTH_INFORMATION const *healthInformation)
{
    return ReportDeployedServicePackageHealth2(healthInformation, nullptr);
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportApplicationHealth2(
    FABRIC_HEALTH_INFORMATION const *healthInformation,
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions)
{
    if (!healthInformation)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthInformation healthInfoObj;
    auto error = healthInfoObj.FromCommonPublicApi(*healthInformation);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptionsObj;
    if (sendOptions != nullptr)
    {
        sendOptionsObj = make_unique<HealthReportSendOptions>();
        error = sendOptionsObj->FromPublicApi(*sendOptions);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    error = codePackageActivationContextSPtr_->ReportApplicationHealth(move(healthInfoObj), move(sendOptionsObj));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportDeployedApplicationHealth2(
    FABRIC_HEALTH_INFORMATION const *healthInformation,
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions)
{
    if (!healthInformation)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthInformation healthInfoObj;
    auto error = healthInfoObj.FromCommonPublicApi(*healthInformation);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptionsObj;
    if (sendOptions != nullptr)
    {
        sendOptionsObj = make_unique<HealthReportSendOptions>();
        error = sendOptionsObj->FromPublicApi(*sendOptions);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    error = codePackageActivationContextSPtr_->ReportDeployedApplicationHealth(move(healthInfoObj), move(sendOptionsObj));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::ReportDeployedServicePackageHealth2(
    FABRIC_HEALTH_INFORMATION const *healthInformation,
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions)
{
    if (!healthInformation)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HealthInformation healthInfoObj;
    auto error = healthInfoObj.FromCommonPublicApi(*healthInformation);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }

    HealthReportSendOptionsUPtr sendOptionsObj;
    if (sendOptions != nullptr)
    {
        sendOptionsObj = make_unique<HealthReportSendOptions>();
        error = sendOptionsObj->FromPublicApi(*sendOptions);
        if (!error.IsSuccess())
        {
            return ComUtility::OnPublicApiReturn(move(error));
        }
    }

    error = codePackageActivationContextSPtr_->ReportDeployedServicePackageHealth(move(healthInfoObj), move(sendOptionsObj));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT STDMETHODCALLTYPE ComCodePackageActivationContext::GetDirectory(
    /* [in] */ LPCWSTR logicalDirectoryName,
    /* [out, retval] */ IFabricStringResult **directoryPath)
{
    wstring logicalDirectoryNameStr;
    HRESULT hr = StringUtility::LpcwstrToWstring(logicalDirectoryName, false /* acceptsNull */, logicalDirectoryNameStr);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    wstring directoryNameString(logicalDirectoryNameStr);
    wstring result;
    auto error = codePackageActivationContextSPtr_->GetDirectory(directoryNameString, result);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(result, directoryPath));
}
