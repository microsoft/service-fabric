// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// ComFabricNodeContextResult::ComFabricNodeContextResult Implementation
//

ComFabricNodeContextResult::ComFabricNodeContextResult(
    FabricNodeContextResultImplSPtr impl)
    : IFabricNodeContextResult2(),
    ComUnknownBase(),
    nodeContextImplSPtr_(impl)
{
}

ComFabricNodeContextResult::~ComFabricNodeContextResult()
{
}

const FABRIC_NODE_CONTEXT * ComFabricNodeContextResult::get_NodeContext( void)
{
    return nodeContextImplSPtr_->get_NodeContext();
}

HRESULT STDMETHODCALLTYPE ComFabricNodeContextResult::GetDirectory(
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
    Common::ErrorCode error = nodeContextImplSPtr_->GetDirectory(directoryNameString, result);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
    return ComUtility::OnPublicApiReturn(ComStringResult::ReturnStringResult(result, directoryPath));
}
