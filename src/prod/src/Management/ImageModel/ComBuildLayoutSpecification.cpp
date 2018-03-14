// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComBuildLayoutSpecification::FabricCreateBuildLayoutSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **buildLayoutSpecification)
{
    ComPointer<ComBuildLayoutSpecification> comBuildLayoutSpecificationCPtr = make_com<ComBuildLayoutSpecification>();
    return comBuildLayoutSpecificationCPtr->QueryInterface(riid, (LPVOID*)buildLayoutSpecification);
}

ComBuildLayoutSpecification::ComBuildLayoutSpecification()
    : buildLayout_()
{
}

ComBuildLayoutSpecification::~ComBuildLayoutSpecification()
{
}

HRESULT ComBuildLayoutSpecification::SetRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    buildLayout_.Root = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}
        
HRESULT ComBuildLayoutSpecification::GetRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(buildLayout_.Root, value));
}


HRESULT ComBuildLayoutSpecification::GetApplicationManifestFile( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetApplicationManifestFile(),
            result));
}

HRESULT ComBuildLayoutSpecification::GetServiceManifestFile( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetServiceManifestFile(parsed_serviceManifestName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetServiceManifestChecksumFile( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetServiceManifestChecksumFile(parsed_serviceManifestName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetCodePackageFolder( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR codePackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetCodePackageFolder(
                parsed_serviceManifestName,
                parsed_codePackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetCodePackageChecksumFile( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR codePackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(codePackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetCodePackageChecksumFile(
                parsed_serviceManifestName,
                parsed_codePackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetConfigPackageFolder( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR configPackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetConfigPackageFolder(
                parsed_serviceManifestName,
                parsed_configPackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetConfigPackageChecksumFile( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR configPackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(configPackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetConfigPackageChecksumFile(
                parsed_serviceManifestName,
                parsed_configPackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetDataPackageFolder( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR dataPackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetDataPackageFolder(
                parsed_serviceManifestName,
                parsed_dataPackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetDataPackageChecksumFile( 
    /* [in] */ LPCWSTR serviceManifestName,
    /* [in] */ LPCWSTR dataPackageName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(serviceManifestName)
    TRY_COM_PARSE_PUBLIC_STRING(dataPackageName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetDataPackageChecksumFile(
                parsed_serviceManifestName,
                parsed_dataPackageName),
            result));
}

HRESULT ComBuildLayoutSpecification::GetSettingsFile( 
    /* [in] */ LPCWSTR configPackageFolder,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(configPackageFolder)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetSettingsFile(parsed_configPackageFolder),
            result));
}

HRESULT ComBuildLayoutSpecification::GetSubPackageArchiveFile( 
    /* [in] */ LPCWSTR packageFolder,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(packageFolder)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetSubPackageArchiveFile(parsed_packageFolder),
            result));
}

HRESULT ComBuildLayoutSpecification::GetChecksumFile( 
    /* [in] */ LPCWSTR fileOrDirectoryName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(fileOrDirectoryName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            buildLayout_.GetChecksumFile(parsed_fileOrDirectoryName),
            result));
}
