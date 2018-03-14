// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

ListReply::ListReply()
    : files_()
    , folders_()
    , primaryStoreShareLocation_()
    , continuationToken_()
{
}

ListReply::ListReply(
    std::vector<StoreFileInfo> const & files,
    std::vector<StoreFolderInfo> const & folders,
    std::wstring const & primaryStoreShareLocation)
    : files_(files)
    , folders_(folders)
    , primaryStoreShareLocation_(primaryStoreShareLocation)
    , continuationToken_()
{
}

ListReply::ListReply(
    std::vector<StoreFileInfo> && files,
    std::vector<StoreFolderInfo> && folders,
    std::wstring const & primaryStoreShareLocation)
    : files_(move(files))
    , folders_(move(folders))
    , primaryStoreShareLocation_(primaryStoreShareLocation)
    , continuationToken_()
{
}

ListReply::ListReply(
    vector<StoreFileInfo> const & files,
    vector<StoreFolderInfo> const & folders,
    wstring const & primaryStoreShareLocation,
    wstring const & continuationToken)
    : files_(files)
    , folders_(folders)
    , primaryStoreShareLocation_(primaryStoreShareLocation)
    , continuationToken_(continuationToken)
{
}

ListReply::ListReply(
    vector<StoreFileInfo> && files,
    vector<StoreFolderInfo> && folders,
    wstring const & primaryStoreShareLocation,
    wstring && continuationToken)
    : files_(move(files))
    , folders_(move(folders))
    , primaryStoreShareLocation_(primaryStoreShareLocation)
    , continuationToken_(move(continuationToken))
{
}
