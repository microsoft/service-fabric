// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management::ResourceManager;

Claim::Claim(
    ResourceIdentifier const & resourceId,
    std::vector<ResourceIdentifier> const & consumerIds)
    : resourceId_(resourceId)
    , consumerIds_(consumerIds)
{
}

bool Claim::IsEmpty() const
{
    return (this->resourceId_.IsEmpty()) || (this->consumerIds_.empty());
}
