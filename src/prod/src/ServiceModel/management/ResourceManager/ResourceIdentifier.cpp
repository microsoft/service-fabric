// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Serialization;
using namespace Management::ResourceManager;
using namespace std;

GlobalWString ResourceIdentifier::InvalidResourceType = make_global<wstring>(L"InvalidResourceType");

ResourceIdentifier::ResourceIdentifier()
    : resourceType_(InvalidResourceType)
    , resourceName_()
{
}

ResourceIdentifier::ResourceIdentifier(wstring const & resourceType)
    : resourceType_(resourceType)
    , resourceName_()
{

}

ResourceIdentifier::ResourceIdentifier(wstring const & resourceType, wstring const & resourceName)
    : resourceType_(resourceType)
    , resourceName_(resourceName)
{
}

ResourceIdentifier::ResourceIdentifier(ResourceIdentifier const & resourceId)
    : FabricSerializable()
    , resourceName_(resourceId.resourceName_)
    , resourceType_(resourceId.resourceType_)
{
}

bool ResourceIdentifier::operator<(ResourceIdentifier const & other) const
{
    return (this->resourceType_ + this->resourceName_) < (other.resourceType_ + other.resourceName_);
}

bool ResourceIdentifier::operator>(ResourceIdentifier const & other) const
{
    return (this->resourceType_ + this->resourceName_) > (other.resourceType_ + other.resourceName_);
}

bool ResourceIdentifier::operator==(ResourceIdentifier const & other) const
{
    return this->resourceType_.compare(other.resourceType_) == 0
        && this->resourceName_.compare(other.resourceName_) == 0;
}

bool ResourceIdentifier::operator!=(ResourceIdentifier const & other) const
{
    return !(*this == other);
}

bool ResourceIdentifier::IsEmpty() const
{
    // If resource name is empty, no matter if type is empty or not,
    // the resource identifier is treated as empty.
    return this->resourceName_.empty();
}

void ResourceIdentifier::WriteTo(__in TextWriter & writer, FormatOptions const & options) const
{
    UNREFERENCED_PARAMETER(options);

    writer.Write("ResId:{0}#{1}#", this->ResourceType, this->ResourceName);
}

size_t hash<ResourceIdentifier>::operator()(ResourceIdentifier const & resourceId) const noexcept
{
    return hash<wstring>{}(resourceId.ResourceType + resourceId.ResourceName);
}