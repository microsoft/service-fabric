// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Management::ResourceManager;

ResourceMetadata::ResourceMetadata()
	: resourceType_(ResourceIdentifier::InvalidResourceType)
	, metadata_()
{
}

ResourceMetadata::ResourceMetadata(wstring const & resourceType)
	: resourceType_(resourceType)
	, metadata_()
{
}
