// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ImageStoreProvisionApplicationTypeDescription");

ImageStoreProvisionApplicationTypeDescription::ImageStoreProvisionApplicationTypeDescription() 
    : ProvisionApplicationTypeDescriptionBase(FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH)
    , appTypeBuildPath_()
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{ 
}

ImageStoreProvisionApplicationTypeDescription::~ImageStoreProvisionApplicationTypeDescription()
{
}

Common::ErrorCode ImageStoreProvisionApplicationTypeDescription::FromPublicApi(
    FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription)
{
    // Build path can have up to MaxFilePathSize length
    TRY_PARSE_PUBLIC_STRING_PER_SIZE_NOT_NULL(publicDescription.BuildPath, appTypeBuildPath_, ParameterValidator::MaxFilePathSize);

    isAsync_ = (publicDescription.Async == TRUE);

    if (publicDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_EX1*>(publicDescription.Reserved);
        applicationPackageCleanupPolicy_ = ApplicationPackageCleanupPolicy::FromPublicApi(ex1->ApplicationPackageCleanupPolicy);
    }

    return ErrorCode::Success();
}
