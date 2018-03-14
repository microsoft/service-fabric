// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

ListRequest::ListRequest()
    : storeRelativePath_()
    , shouldIncludeDetails_(false)
    , isRecursive_(false)
    , continuationToken_()
    , isPaging_(false)
{
}

ListRequest::ListRequest(std::wstring const & storeRelativePath)
    : storeRelativePath_(storeRelativePath)
    , shouldIncludeDetails_(false)
    , isRecursive_(false)
    , continuationToken_()
    , isPaging_(false)
{
}

ListRequest::ListRequest(
    std::wstring const & storeRelativePath,
    BOOLEAN const & shouldIncludeDetails,
    BOOLEAN const & isRecursive)
    : storeRelativePath_(storeRelativePath)
    , shouldIncludeDetails_(shouldIncludeDetails)
    , isRecursive_(isRecursive)
    , continuationToken_()
    , isPaging_(false)
{
}

ListRequest::ListRequest(
    wstring const & storeRelativePath,
    BOOLEAN const & shouldIncludeDetails,
    BOOLEAN const & isRecursive,
    wstring const & continuationToken,
    bool isPaging)
    : storeRelativePath_(storeRelativePath)
    , shouldIncludeDetails_(shouldIncludeDetails)
    , isRecursive_(isRecursive)
    , continuationToken_(continuationToken)
    , isPaging_(isPaging)
{
}
