// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

using namespace Management::ResourceManager;

ResourceDataItem::ResourceDataItem()
	: StoreData()
	, name_()
	, references_()
{
}

ResourceDataItem::ResourceDataItem(wstring const & name)
	: StoreData()
	, name_(name)
	, references_()
{
}

ResourceDataItem::ResourceDataItem(ResourceIdentifier const & resourceId)
	: StoreData()
	, name_(resourceId.ResourceName)
	, references_()
{
}

void ResourceDataItem::WriteTo(__in TextWriter & w, FormatOptions const & options) const
{
	UNREFERENCED_PARAMETER(options);

	w.Write("RessourceDataItem:{0}", this->Name);
}