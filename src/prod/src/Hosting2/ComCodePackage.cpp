// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ComCodePackage::ComCodePackage(
    ServiceModel::CodePackageDescription const & codePackageDescription,
    std::wstring const& serviceManifestName,
    std::wstring const& serviceManifestVersion, 
    std::wstring const& path,
    ServiceModel::RunAsPolicyDescription const & setupRunAsPolicyDescription,
    ServiceModel::RunAsPolicyDescription const & runAsPolicyDescription)
    : heap_(),
    codePackageDescription_(),
    runAsPolicy_(),
    setupRunAsPolicy_(),
    path_(path)
{
    codePackageDescription_ = heap_.AddItem<FABRIC_CODE_PACKAGE_DESCRIPTION>();
    codePackageDescription.ToPublicApi(heap_, serviceManifestName, serviceManifestVersion, *codePackageDescription_);

    if (!setupRunAsPolicyDescription.UserRef.empty())
    {
        setupRunAsPolicy_ = heap_.AddItem<FABRIC_RUNAS_POLICY_DESCRIPTION>();
        setupRunAsPolicyDescription.ToPublicApi(heap_, *setupRunAsPolicy_);
    }

    if (!runAsPolicyDescription.UserRef.empty())
    {
        runAsPolicy_ = heap_.AddItem<FABRIC_RUNAS_POLICY_DESCRIPTION>();
        runAsPolicyDescription.ToPublicApi(heap_, *runAsPolicy_);
    }
}

ComCodePackage::~ComCodePackage()
{
}

const FABRIC_CODE_PACKAGE_DESCRIPTION * ComCodePackage::get_Description(void)
{
    return codePackageDescription_.GetRawPointer();
}

LPCWSTR ComCodePackage::get_Path(void)
{
    return path_.c_str();
}

const FABRIC_RUNAS_POLICY_DESCRIPTION * ComCodePackage::get_SetupEntryPointRunAsPolicy(void)
{
    return setupRunAsPolicy_.GetRawPointer();
}
        
const FABRIC_RUNAS_POLICY_DESCRIPTION * ComCodePackage::get_EntryPointRunAsPolicy(void)
{
    return runAsPolicy_.GetRawPointer();
}
