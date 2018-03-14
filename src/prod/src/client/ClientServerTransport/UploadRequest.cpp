// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

UploadRequest::UploadRequest()
    : stagingRelativePath_()
    , storeRelativePath_()
    , shouldOverwrite_()
{
}

UploadRequest::UploadRequest(
    std::wstring const & stagingRelativePath,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite) 
    : stagingRelativePath_(stagingRelativePath)
    , storeRelativePath_(storeRelativePath)
    , shouldOverwrite_(shouldOverwrite)
{
}

void UploadRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("UploadRequest{StagingRelativePath={0}, StoreRelativePath={1}, ShouldOverwrite={2}}", stagingRelativePath_, storeRelativePath_, shouldOverwrite_);
}
