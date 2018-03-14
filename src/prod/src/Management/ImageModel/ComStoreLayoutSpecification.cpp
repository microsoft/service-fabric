// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComStoreLayoutSpecification::FabricCreateStoreLayoutSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **storeLayoutSpecification)
{
    ComPointer<ComStoreLayoutSpecification> comStoreLayoutSpecificationCPtr = make_com<ComStoreLayoutSpecification>();
    return comStoreLayoutSpecificationCPtr->QueryInterface(riid, (LPVOID*)storeLayoutSpecification);
}

ComStoreLayoutSpecification::ComStoreLayoutSpecification()
    : storeLayout_()
{
}

ComStoreLayoutSpecification::~ComStoreLayoutSpecification()
{
}

HRESULT ComStoreLayoutSpecification::SetRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    storeLayout_.Root = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}
        
HRESULT ComStoreLayoutSpecification::GetRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(storeLayout_.Root, value));
}

HRESULT  ComStoreLayoutSpecification::GetApplicationManifestFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationTypeVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetApplicationManifestFile(
                parsed_applicationTypeName,
                parsed_applicationTypeVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetApplicationInstanceFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR applicationInstanceVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(applicationInstanceVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetApplicationInstanceFile(
                parsed_applicationTypeName,
                parsed_applicationId,
                parsed_applicationInstanceVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetApplicationPackageFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR applicationRolloutVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(applicationRolloutVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetApplicationPackageFile(
                parsed_applicationTypeName,
                parsed_applicationId,
                parsed_applicationRolloutVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetServicePackageFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR applicationId,
    /* [in] */ LPCWSTR servicePackageName,
    /* [in] */ LPCWSTR servicePackageRolloutVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(applicationId)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(servicePackageRolloutVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetServicePackageFile(
                parsed_applicationTypeName,
                parsed_applicationId,
                parsed_servicePackageName,
                parsed_servicePackageRolloutVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetServiceManifestFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR serviceManifestVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetServiceManifestFile(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_serviceManifestVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetServiceManifestChecksumFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR serviceManifestVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetServiceManifestChecksumFile(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_serviceManifestVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetCodePackageFolder( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR codePackageName,
    /* [in] */ LPCWSTR codePackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetCodePackageFolder(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_codePackageName,
                parsed_codePackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetCodePackageChecksumFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR codePackageName,
    /* [in] */ LPCWSTR codePackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetCodePackageChecksumFile(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_codePackageName,
                parsed_codePackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetConfigPackageFolder( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR configPackageName,
    /* [in] */ LPCWSTR configPackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetConfigPackageFolder(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_configPackageName,
                parsed_configPackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetConfigPackageChecksumFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR configPackageName,
    /* [in] */ LPCWSTR configPackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetConfigPackageChecksumFile(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_configPackageName,
                parsed_configPackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetDataPackageFolder( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR dataPackageName,
    /* [in] */ LPCWSTR dataPackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetDataPackageFolder(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_dataPackageName,
                parsed_dataPackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetDataPackageChecksumFile( 
    /* [in] */ LPCWSTR applicationTypeName,
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR dataPackageName,
    /* [in] */ LPCWSTR dataPackageVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(applicationTypeName)
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageVersion)
    
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetDataPackageChecksumFile(
                parsed_applicationTypeName,
                parsed_serviceManifestName,
                parsed_dataPackageName,
                parsed_dataPackageVersion),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetSettingsFile( 
    /* [in] */ LPCWSTR configPackageFolder,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(configPackageFolder)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetSettingsFile(parsed_configPackageFolder),
            result));
}

HRESULT  ComStoreLayoutSpecification::GetSubPackageArchiveFile( 
    /* [in] */ LPCWSTR packageFolder,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(packageFolder)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            storeLayout_.GetSubPackageArchiveFile(parsed_packageFolder),
            result));
}
