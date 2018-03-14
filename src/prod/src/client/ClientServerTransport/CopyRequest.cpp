// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

CopyRequest::CopyRequest()
: sourceStoreRelativePath_()
, sourceFileVersion_()
, destinationStoreRelativePath_()
, shouldOverwrite_()
{
}

CopyRequest::CopyRequest(
    std::wstring const & sourceStoreRelativePath,
    StoreFileVersion const & sourceFileVersion,
    std::wstring const & destinationRelativePath,
    bool const shouldOverwrite)
    : sourceStoreRelativePath_(sourceStoreRelativePath)
    , sourceFileVersion_(sourceFileVersion)
    , destinationStoreRelativePath_(destinationRelativePath)
    , shouldOverwrite_(shouldOverwrite)
{
}

void CopyRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("CopyRequest{SourceStoreRelativePath={0}, SourceFileVersion={1}, DestinationStoreRelativePath={2} ShouldOverwrite={3}}", sourceStoreRelativePath_, sourceFileVersion_, destinationStoreRelativePath_, shouldOverwrite_);
}
