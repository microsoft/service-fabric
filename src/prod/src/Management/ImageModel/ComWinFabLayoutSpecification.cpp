// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComWinFabStoreLayoutSpecification::FabricCreateWinFabStoreLayoutSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **fabricLayoutSpecification)
{
    ComPointer<ComWinFabStoreLayoutSpecification> comWinFabStoreLayoutSpecificationCPtr = make_com<ComWinFabStoreLayoutSpecification>();
    return comWinFabStoreLayoutSpecificationCPtr->QueryInterface(riid, (LPVOID*)fabricLayoutSpecification);
}

ComWinFabStoreLayoutSpecification::ComWinFabStoreLayoutSpecification()
    : winFabLayout_()
{
}

ComWinFabStoreLayoutSpecification::~ComWinFabStoreLayoutSpecification()
{
}

HRESULT ComWinFabStoreLayoutSpecification::SetRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    winFabLayout_.Root = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComWinFabStoreLayoutSpecification::GetRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(winFabLayout_.Root, value));
}

HRESULT ComWinFabStoreLayoutSpecification::GetPatchFile( 
    /* [in] */ LPCWSTR version,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(version)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabLayout_.GetPatchFile(parsed_version),
        result));
}

HRESULT ComWinFabStoreLayoutSpecification::GetCabPatchFile(
    /* [in] */ LPCWSTR version,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(version)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabLayout_.GetCabPatchFile(parsed_version),
        result));
}

HRESULT ComWinFabStoreLayoutSpecification::GetCodePackageFolder(
    /* [in] */ LPCWSTR version,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(version)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabLayout_.GetCodePackageFolder(parsed_version),
        result));
}

HRESULT ComWinFabStoreLayoutSpecification::GetClusterManifestFile( 
    /* [in] */ LPCWSTR clusterManifestVersion,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(clusterManifestVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabLayout_.GetClusterManifestFile(parsed_clusterManifestVersion),
        result));
}
