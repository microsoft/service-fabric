// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

SendAvailableContainerImagesEventArgs::SendAvailableContainerImagesEventArgs(
    std::vector<wstring> const & availableImages,
    std::wstring const & nodeId)
    : availableImages_(availableImages),
    nodeId_(nodeId)
{
}

SendAvailableContainerImagesEventArgs::SendAvailableContainerImagesEventArgs(SendAvailableContainerImagesEventArgs const & other)
    : availableImages_(other.availableImages_),
    nodeId_(other.nodeId_)
{
}

SendAvailableContainerImagesEventArgs::SendAvailableContainerImagesEventArgs(SendAvailableContainerImagesEventArgs && other)
    : availableImages_(move(other.availableImages_)),
    nodeId_(move(other.nodeId_))
{
}

SendAvailableContainerImagesEventArgs const & SendAvailableContainerImagesEventArgs::operator = (SendAvailableContainerImagesEventArgs const & other)
{
    if (this != &other)
    {
        this->availableImages_ = other.availableImages_;
        this->nodeId_ = other.nodeId_;
    }
    return *this;
}

SendAvailableContainerImagesEventArgs const & SendAvailableContainerImagesEventArgs::operator = (SendAvailableContainerImagesEventArgs && other)
{
    if (this != &other)
    {
        this->availableImages_ = move(other.availableImages_);
        this->nodeId_ = move(other.nodeId_);
    }
    return *this;
}

void SendAvailableContainerImagesEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("SendAvailableContainerImagesEventArgs { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("Images {");
    for (auto & image : availableImages_)
    {
        w.Write("Image = {0} ", image);
    }
    w.Write("}");
    w.Write("}");
}
