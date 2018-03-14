// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;


HRESULT ComWinFabRunLayoutSpecification::FabricCreateWinFabRunLayoutSpecification( 
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **fabricLayoutSpecification)
{
    ComPointer<ComWinFabRunLayoutSpecification> comWinFabRunLayoutSpecificationCPtr = make_com<ComWinFabRunLayoutSpecification>();
    return comWinFabRunLayoutSpecificationCPtr->QueryInterface(riid, (LPVOID*)fabricLayoutSpecification);
}

ComWinFabRunLayoutSpecification::ComWinFabRunLayoutSpecification()
    : winFabRunLayout_()
{
}

ComWinFabRunLayoutSpecification::~ComWinFabRunLayoutSpecification()
{
}

HRESULT ComWinFabRunLayoutSpecification::SetRoot( 
    /* [in] */ LPCWSTR root)
{
    TRY_COM_PARSE_PUBLIC_FILEPATH(root)

    winFabRunLayout_.Root = parsed_root;

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComWinFabRunLayoutSpecification::GetRoot(
    /* [retval][out] */ IFabricStringResult **value)
{
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(winFabRunLayout_.Root, value));
}

HRESULT ComWinFabRunLayoutSpecification::GetPatchFile( 
    /* [in] */ LPCWSTR codeVersion,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(codeVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabRunLayout_.GetPatchFile(parsed_codeVersion),
        result));
}

HRESULT ComWinFabRunLayoutSpecification::GetCabPatchFile(
    /* [in] */ LPCWSTR codeVersion,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(codeVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabRunLayout_.GetCabPatchFile(parsed_codeVersion),
        result));
}

HRESULT ComWinFabRunLayoutSpecification::GetCodePackageFolder(
    /* [in] */ LPCWSTR codeVersion,
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(codeVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabRunLayout_.GetCodePackageFolder(parsed_codeVersion),
        result));
}

HRESULT ComWinFabRunLayoutSpecification::GetClusterManifestFile( 
    /* [in] */ LPCWSTR clusterManifestVersion,   
    /* [retval][out] */ IFabricStringResult **result)
{
    TRY_COM_PARSE_PUBLIC_STRING(clusterManifestVersion)

    return ComUtility::OnPublicApiReturn(
        ComStringResult::ReturnStringResult(
        winFabRunLayout_.GetClusterManifestFile(parsed_clusterManifestVersion),
        result));
}
