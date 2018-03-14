// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComRunLayoutSpecification::FabricCreateRunLayoutSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **runLayoutSpecification)
{
    ComPointer<ComRunLayoutSpecification> comRunLayoutSpecificationCPtr = make_com<ComRunLayoutSpecification>();
    return comRunLayoutSpecificationCPtr->QueryInterface(riid, (LPVOID*)runLayoutSpecification);
}

ComRunLayoutSpecification::ComRunLayoutSpecification()
    : runLayout_()
{
}

ComRunLayoutSpecification::~ComRunLayoutSpecification()
{
}

HRESULT ComRunLayoutSpecification::SetRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    runLayout_.Root = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}
        
HRESULT ComRunLayoutSpecification::GetRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(runLayout_.Root, value));
}

HRESULT ComRunLayoutSpecification::GetApplicationFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetApplicationFolder(parsed_applicationId),
            result));
}

HRESULT ComRunLayoutSpecification::GetApplicationWorkFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetApplicationWorkFolder(parsed_applicationId),
            result));
}

HRESULT ComRunLayoutSpecification::GetApplicationLogFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetApplicationLogFolder(parsed_applicationId),
            result));
}

HRESULT ComRunLayoutSpecification::GetApplicationTempFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetApplicationTempFolder(parsed_applicationId),
            result));
}

HRESULT  ComRunLayoutSpecification::GetApplicationPackageFile( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR applicationRolloutVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(applicationRolloutVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetApplicationPackageFile(
                parsed_applicationId,
                parsed_applicationRolloutVersion),
            result));
}

HRESULT ComRunLayoutSpecification::GetServicePackageFile( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR servicePackageRolloutVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageRolloutVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetServicePackageFile(
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_servicePackageRolloutVersion),
            result));
}

HRESULT ComRunLayoutSpecification::GetCurrentServicePackageFile( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetCurrentServicePackageFile(
                parsed_applicationId,
                parsed_servicePackageName,
                wstring()),
            result));
}

HRESULT ComRunLayoutSpecification::GetCurrentServicePackageFile2(
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR servicePackageActivationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId);
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName);
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageActivationId);
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetCurrentServicePackageFile(
            parsed_applicationId,
            parsed_servicePackageName,
            parsed_servicePackageActivationId),
        result));
}

HRESULT ComRunLayoutSpecification::GetEndpointDescriptionsFile( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetEndpointDescriptionsFile(
                parsed_applicationId,
                parsed_servicePackageName,
                wstring()),
            result));
}

HRESULT STDMETHODCALLTYPE ComRunLayoutSpecification::GetEndpointDescriptionsFile2(
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR servicePackageActivationId,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId);
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName);
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageActivationId);
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetEndpointDescriptionsFile(
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_servicePackageActivationId),
            result));
}

HRESULT ComRunLayoutSpecification::GetServiceManifestFile( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR serviceManifestVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetServiceManifestFile(
                parsed_applicationId,
                parsed_serviceManifestName,
                parsed_serviceManifestVersion),
            result));
}

HRESULT ComRunLayoutSpecification::GetCodePackageFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR codePackageName,
    /* [in] */ LPCWSTR codePackageVersion,
    /* [in] */ BOOLEAN isSharedPackage,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetCodePackageFolder(
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_codePackageName,
                parsed_codePackageVersion,
                isSharedPackage == 1),
            result));
}

HRESULT ComRunLayoutSpecification::GetConfigPackageFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR configPackageName,
    /* [in] */ LPCWSTR configPackageVersion,
    /* [in] */ BOOLEAN isSharedPackage,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetConfigPackageFolder(
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_configPackageName,
                parsed_configPackageVersion,
                isSharedPackage == 1),
            result));
}

HRESULT ComRunLayoutSpecification::GetDataPackageFolder( 
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR dataPackageName,
    /* [in] */ LPCWSTR dataPackageVersion,
    /* [in] */ BOOLEAN isSharedPackage,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetDataPackageFolder(
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_dataPackageName,
                parsed_dataPackageVersion,
                isSharedPackage == 1),
            result));
}

HRESULT ComRunLayoutSpecification::GetSettingsFile( 
    /* [in] */ LPCWSTR configPackageFolder,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(configPackageFolder)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            runLayout_.GetSettingsFile(parsed_configPackageFolder),
            result));
}
