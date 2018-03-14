// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DownloadContainerImagesRequest::DownloadContainerImagesRequest()
    : containerImages_()
    , timeoutTicks_()
{
}

DownloadContainerImagesRequest::DownloadContainerImagesRequest(
    std::vector<ContainerImageDescription> const & containerImages,
    int64 timeoutTicks)
    : containerImages_(containerImages)
    , timeoutTicks_(timeoutTicks)
{
}

void DownloadContainerImagesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Container Images { ");
    
    for (auto image : containerImages_)
    {
        w.Write("Image = {0}", image);
    }

    w.Write("TimeoutTicks = {0}", timeoutTicks_);
    w.Write("}");
}
