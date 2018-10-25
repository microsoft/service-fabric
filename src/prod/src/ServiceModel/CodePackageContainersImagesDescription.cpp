// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

CodePackageContainersImagesDescription::CodePackageContainersImagesDescription()
    : containersImages_()
{
}

CodePackageContainersImagesDescription::CodePackageContainersImagesDescription(CodePackageContainersImagesDescription const & other)
    : containersImages_(other.ContainersImages)
{
}

CodePackageContainersImagesDescription::CodePackageContainersImagesDescription(CodePackageContainersImagesDescription && other)
    : containersImages_(move(other.ContainersImages))
{
}

CodePackageContainersImagesDescription const & CodePackageContainersImagesDescription::operator=(CodePackageContainersImagesDescription const & other)
{
    if (this != &other)
    {
        containersImages_ = other.containersImages_;
    }
    return *this;
}

CodePackageContainersImagesDescription const & CodePackageContainersImagesDescription::operator=(CodePackageContainersImagesDescription && other)
{
    if (this != &other)
    {
        containersImages_ = move(other.containersImages_);
    }
    return *this;
}

bool CodePackageContainersImagesDescription::operator==(CodePackageContainersImagesDescription const & other) const
{
    return containersImages_ == other.containersImages_;
}

bool CodePackageContainersImagesDescription::operator!=(CodePackageContainersImagesDescription const & other) const
{
    return !(*this == other);
}

void CodePackageContainersImagesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring CodePackageContainersImagesDescription::ToString() const
{
    return wformatString(
        "CodePackageContainersImagesDescription (ContainersImages = {0})", ContainersImages);
}

void CodePackageContainersImagesDescription::SetContainersImages(std::vector<std::wstring> const& containersImages)
{
    containersImages_.clear();
    for (auto image : containersImages)
    {
        containersImages_.push_back(image);
    }
}
