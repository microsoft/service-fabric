// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComFabricDeploymentSpecification::FabricCreateFabricDeploymentSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **fabricDeploymentSpecification)
{
    ComPointer<ComFabricDeploymentSpecification> comFabricDeploymentSpecificationCPtr = make_com<ComFabricDeploymentSpecification>();
    return comFabricDeploymentSpecificationCPtr->QueryInterface(riid, (LPVOID*)fabricDeploymentSpecification);
}

ComFabricDeploymentSpecification::ComFabricDeploymentSpecification()
    : fabricDeployment_()
{
}

ComFabricDeploymentSpecification::~ComFabricDeploymentSpecification()
{
}

HRESULT ComFabricDeploymentSpecification::SetDataRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    fabricDeployment_.DataRoot = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}
        
HRESULT ComFabricDeploymentSpecification::GetDataRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(fabricDeployment_.DataRoot, value));
}

HRESULT ComFabricDeploymentSpecification::SetLogRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    fabricDeployment_.LogRoot = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}
        
HRESULT ComFabricDeploymentSpecification::GetLogRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(fabricDeployment_.LogRoot, value));
}

HRESULT ComFabricDeploymentSpecification::GetLogFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetLogFolder(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetTracesFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetTracesFolder(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetCrashDumpsFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetCrashDumpsFolder(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetApplicationCrashDumpsFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetApplicationCrashDumpsFolder(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetAppInstanceDataFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetAppInstanceDataFolder(),
            result));
}
;
HRESULT ComFabricDeploymentSpecification::GetPerformanceCountersBinaryFolder( 
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetPerformanceCountersBinaryFolder(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetTargetInformationFile(     
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetTargetInformationFile(),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetNodeFolder( 
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetNodeFolder(parsed_nodeName),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetFabricFolder( 
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetFabricFolder(parsed_nodeName),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetCurrentClusterManifestFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetCurrentClusterManifestFile(parsed_nodeName),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetVersionedClusterManifestFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ LPCWSTR version,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)
    TRY_COM_PARSE_PUBLIC_STRING(version)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetVersionedClusterManifestFile(parsed_nodeName, parsed_version),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetInstallerScriptFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetInstallerScriptFile(parsed_nodeName),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetInstallerLogFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ LPCWSTR codeVersion,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)
    TRY_COM_PARSE_PUBLIC_STRING(codeVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetInstallerLogFile(parsed_nodeName, parsed_codeVersion),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetInfrastructureManfiestFile(
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetInfrastructureManfiestFile(parsed_nodeName),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetConfigurationDeploymentFolder(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ LPCWSTR configVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)
    TRY_COM_PARSE_PUBLIC_STRING(configVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetConfigurationDeploymentFolder(parsed_nodeName, parsed_configVersion),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetDataDeploymentFolder(
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetDataDeploymentFolder(parsed_nodeName),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetCodeDeploymentFolder(
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ LPCWSTR service,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)
    TRY_COM_PARSE_PUBLIC_STRING(service)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetCodeDeploymentFolder(parsed_nodeName, parsed_service),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetInstalledBinaryFolder(
    /* [in] */ LPCWSTR installationFolder,
    /* [in] */ LPCWSTR service,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(installationFolder)
    TRY_COM_PARSE_PUBLIC_STRING(service)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetInstalledBinaryFolder(parsed_installationFolder, parsed_service),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetWorkFolder(
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetWorkFolder(parsed_nodeName),
        result));
}

HRESULT ComFabricDeploymentSpecification::GetCurrentFabricPackageFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
            fabricDeployment_.GetCurrentFabricPackageFile(parsed_nodeName),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetVersionedFabricPackageFile( 
    /* [in] */ LPCWSTR nodeName,
    /* [in] */ LPCWSTR version,
    /* [retval][out] */ IFabricStringResult **result) 
{
    TRY_COM_PARSE_PUBLIC_STRING(nodeName)
    TRY_COM_PARSE_PUBLIC_STRING(version)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        fabricDeployment_.GetVersionedFabricPackageFile(parsed_nodeName, parsed_version),
            result));
}

HRESULT ComFabricDeploymentSpecification::GetQueryTraceFolder(
    /* [retval][out] */ IFabricStringResult **result) 
{
    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(fabricDeployment_.GetQueryTraceFolder(), result));
}

HRESULT ComFabricDeploymentSpecification::GetEnableCircularTraceSession(
    /* [retval][out] */ BOOLEAN *result)
{
    *result = fabricDeployment_.EnableCircularTraceSession;
    return S_OK;
}

HRESULT ComFabricDeploymentSpecification::SetEnableCircularTraceSession(
    /* [in] */ BOOLEAN value)
{
    fabricDeployment_.EnableCircularTraceSession = value;

    return ComUtility::OnPublicApiReturn(S_OK);
}
