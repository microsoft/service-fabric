// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

ImageStoreBaseRequest::ImageStoreBaseRequest()
    : storeRelativePath_()    
{
}

ImageStoreBaseRequest::ImageStoreBaseRequest(wstring const & storeRelativePath)
    : storeRelativePath_(storeRelativePath)
{
}

void ImageStoreBaseRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ImageStoreBaseRequest{StoreRelativePath={0}}", storeRelativePath_);
}
