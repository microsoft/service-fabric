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

StringLiteral const TraceComponent("ProvisionApplicationTypeDescription");

ProvisionApplicationTypeDescription::ProvisionApplicationTypeDescription() 
    : appTypeBuildPath_()
    , isAsync_(false)
    , appTypeName_()
    , appTypeVersion_()
    , applicationPackageDownloadUri_()
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{ 
}

ProvisionApplicationTypeDescription::~ProvisionApplicationTypeDescription()
{
}

ErrorCode ProvisionApplicationTypeDescription::Update(ProvisionApplicationTypeDescriptionBaseSPtr && description)
{
    switch (description->Kind)
    {
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH:
    {
        auto desc = dynamic_cast<ImageStoreProvisionApplicationTypeDescription*>(description.get());
        if (!desc)
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Invalid_Provision_Object));
        }
              
        appTypeBuildPath_ = desc->TakeAppTypeBuildPath();
        applicationPackageCleanupPolicy_ = desc->ApplicationPackageCleanupPolicy;
        break;
    }
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE:
    {
        auto desc = dynamic_cast<ExternalStoreProvisionApplicationTypeDescription*>(description.get());
        if (!desc)
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Invalid_Provision_Object));
        }

        applicationPackageDownloadUri_ = desc->TakeApplicationPackageDownloadUri();
        appTypeName_ = desc->TakeAppTypeName();
        appTypeVersion_ = desc->TakeAppTypeVersion();
        break;
    }
    default:
        return TraceAndGetError(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_CM_RC(Invalid_Provision_Kind), description->Kind));
    }

    isAsync_ = description->IsAsync;
    return ErrorCode::Success();
}

Common::ErrorCode ProvisionApplicationTypeDescription::FromPublicApi(
    FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription)
{
    ImageStoreProvisionApplicationTypeDescription description;
    auto error = description.FromPublicApi(publicDescription);
    if (!error.IsSuccess()) { return error; }

    appTypeBuildPath_ = description.TakeAppTypeBuildPath();
    isAsync_ = description.IsAsync;
    applicationPackageCleanupPolicy_ = description.ApplicationPackageCleanupPolicy;
    return ErrorCode::Success();
}

Common::ErrorCode ProvisionApplicationTypeDescription::FromPublicApi(
    FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription)
{
    ExternalStoreProvisionApplicationTypeDescription description;
    auto error = description.FromPublicApi(publicDescription);
    if (!error.IsSuccess()) { return error; }

    applicationPackageDownloadUri_ = description.TakeApplicationPackageDownloadUri();
    appTypeName_ = description.TakeAppTypeName();
    appTypeVersion_ = description.TakeAppTypeVersion();
    isAsync_ = description.IsAsync;
    return ErrorCode::Success();
}

Common::ErrorCode ProvisionApplicationTypeDescription::FromPublicApi(
    FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE const & publicDescriptionBase)
{
    switch (publicDescriptionBase.Kind)
    {
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH:
    {
        auto descrValue = static_cast<FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION*>(publicDescriptionBase.Value);
        return FromPublicApi(*descrValue);
    }
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE:
    {
        auto descrValue = static_cast<FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION*>(publicDescriptionBase.Value);
        return FromPublicApi(*descrValue);
    }
    default:
        return TraceAndGetError(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_CM_RC(Invalid_Provision_Kind), publicDescriptionBase.Kind));
    }
}

Common::ErrorCode ProvisionApplicationTypeDescription::CheckIsValid() const
{
    if (applicationPackageDownloadUri_.empty())
    {
        // Validate that build path is specified in this case
        if (appTypeBuildPath_.empty())
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Provision_Missing_Required_Parameter));
        }
    }
    else
    {
        // Only specify one of the build path or download path.
        if (!appTypeBuildPath_.empty())
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_CM_RC(Provision_BuildPathAndDownloadPathAreExclusive), appTypeBuildPath_, applicationPackageDownloadUri_));
        }

        auto error = ExternalStoreProvisionApplicationTypeDescription::IsValidSfpkgDownloadPath(applicationPackageDownloadUri_);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "IsValidSfpkgDownloadPath returned {0}: {1}", error, error.Message);
            return error;
        }

        // Application type name and version must be specified
        if (appTypeName_.empty() || appTypeVersion_.empty())
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                GET_CM_RC(Provision_Missing_AppTypeInfo));
        }
    }

    return ErrorCode::Success();
}

ErrorCode ProvisionApplicationTypeDescription::TraceAndGetError(
    ErrorCodeValue::Enum errorValue,
    wstring && message) const
{
    Trace.WriteWarning(TraceComponent, "{0}: {1}", errorValue, message);
    return ErrorCode(errorValue, move(message));
}
