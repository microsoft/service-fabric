// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;

FileUploadCreateRequestHeader::FileUploadCreateRequestHeader(std::wstring const & serviceName, std::wstring const & storeRelativePath, bool const shouldOverwrite, uint64 fileSize)
    : serviceName_(serviceName)
    , storeRelativePath_(storeRelativePath)
    , shouldOverwrite_(shouldOverwrite)
    , fileSize_(fileSize)
{
}

void FileUploadCreateRequestHeader::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.Write("FileMetadataHeader{ ");
    w.Write("ServiceName = {0},", serviceName_);
    w.Write("StoreRelativePath = {0},", storeRelativePath_);
    w.Write("ShouldOvewrite = {0},", shouldOverwrite_);
    w.Write("FileSize = {0},", fileSize_);
    w.Write(" }");
}