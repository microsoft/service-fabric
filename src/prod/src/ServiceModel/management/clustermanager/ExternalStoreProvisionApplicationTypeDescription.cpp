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

StringLiteral const TraceComponent("ExternalStoreProvisionApplicationTypeDescription");

ExternalStoreProvisionApplicationTypeDescription::ExternalStoreProvisionApplicationTypeDescription() 
    : ProvisionApplicationTypeDescriptionBase(FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE)
    , appTypeName_()
    , appTypeVersion_()
    , applicationPackageDownloadUri_()
{ 
}

ExternalStoreProvisionApplicationTypeDescription::~ExternalStoreProvisionApplicationTypeDescription()
{
}

Common::ErrorCode ExternalStoreProvisionApplicationTypeDescription::FromPublicApi(
    FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription)
{
    isAsync_ = (publicDescription.Async == TRUE);

    TRY_PARSE_PUBLIC_STRING_PER_SIZE_NOT_NULL(publicDescription.ApplicationPackageDownloadUri, applicationPackageDownloadUri_, ParameterValidator::MaxFilePathSize);
    TRY_PARSE_PUBLIC_STRING(publicDescription.ApplicationTypeVersion, appTypeVersion_);
    TRY_PARSE_PUBLIC_STRING(publicDescription.ApplicationTypeName, appTypeName_);
    
    return CheckIsValid();
}

Common::ErrorCode ExternalStoreProvisionApplicationTypeDescription::CheckIsValid() const
{
    if (applicationPackageDownloadUri_.empty())
    {
        return TraceAndGetError(
            ErrorCodeValue::InvalidArgument,
            GET_CM_RC(Provision_Missing_Required_Parameter));
    }
    else
    {
        // Only specify one of the build path or download path.
        auto error = IsValidSfpkgDownloadPath(applicationPackageDownloadUri_);
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

ErrorCode ExternalStoreProvisionApplicationTypeDescription::IsValidSfpkgDownloadPath(std::wstring const & downloadPathString)
{
    Uri downloadPath;
    if (!Uri::TryParse(downloadPathString, downloadPath))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_CM_RC(Provision_Invalid_DownloadPath), downloadPathString));
    }

    // Check protocol: accept only http and https
    if (!StringUtility::AreEqualCaseInsensitive(downloadPath.Scheme, L"http") &&
        !StringUtility::AreEqualCaseInsensitive(downloadPath.Scheme, L"https"))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_CM_RC(Provision_Invalid_DownloadPath), downloadPathString));
    }

    // Check .sfpkg extension
    if (!StringUtility::EndsWithCaseInsensitive(downloadPath.Segments.back(), *Path::SfpkgExtension))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_CM_RC(Provision_Invalid_DownloadPath), downloadPathString));
    }

    return ErrorCode::Success();
}
