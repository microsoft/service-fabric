// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ComDataPackage::ComDataPackage(
    DataPackageDescription const & dataPackageDescription, 
    wstring const& serviceManifestName,
    wstring const& serviceManifestVersion, 
    wstring const& path) 
    : heap_(),
    dataPackageDescription_(),
    path_(path)
{
    dataPackageDescription_ = heap_.AddItem<FABRIC_DATA_PACKAGE_DESCRIPTION>();
    dataPackageDescription.ToPublicApi(heap_, serviceManifestName, serviceManifestVersion, *dataPackageDescription_);
}

ComDataPackage::~ComDataPackage()
{
}

const FABRIC_DATA_PACKAGE_DESCRIPTION * ComDataPackage::get_Description(void)
{
    return dataPackageDescription_.GetRawPointer();
}

LPCWSTR ComDataPackage::get_Path(void)
{
    return path_.c_str();
}
