//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel::ModelV2;
using namespace Common;

INITIALIZE_SIZE_ESTIMATION(ContainerCodePackageDescription);

bool ContainerCodePackageDescription::operator==(ContainerCodePackageDescription const & other) const
{
    if (name_ == other.name_
        && image_ == other.image_
        && imageRegistryCredential_ == other.imageRegistryCredential_
        && entryPoint_ == other.entryPoint_
        && commands_ == other.commands_
        && reliableCollectionsRefs_ == other.reliableCollectionsRefs_
        && volumeRefs_ == other.volumeRefs_
        && volumes_ == other.volumes_
        && endpoints_ == other.endpoints_
        && settings_ == other.settings_
        && environmentVariables_ == other.environmentVariables_
        && resources_ == other.resources_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ContainerCodePackageDescription::CanUpgrade(ContainerCodePackageDescription const & other) const
{
    //if reliable collections or block store is used, allow upgrading the application
    if (reliableCollectionsRefs_.size() > 0 ||
        volumes_.size() > 0)
    {
        return true;
    }

    // The following fields are allowed for upgrading
    // image_
    // setting_
    if (name_ == other.name_
        && imageRegistryCredential_ == other.imageRegistryCredential_
        && entryPoint_ == other.entryPoint_
        && commands_ == other.commands_
        && reliableCollectionsRefs_ == other.reliableCollectionsRefs_
        && volumeRefs_ == other.volumeRefs_
        && volumes_ == other.volumes_
        && endpoints_ == other.endpoints_
        && environmentVariables_ == other.environmentVariables_
        && diagnostics_ == other.diagnostics_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

Common::ErrorCode ContainerCodePackageDescription::TryValidate(wstring const &traceId) const
{
    if (name_.empty())
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    auto nextTraceId = GetNextTraceId(traceId, name_);
    if (image_.empty())
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerImageNotSpecified), nextTraceId));
    }

    StringCollection imageAndTag;
    StringUtility::Split<wstring>(image_, imageAndTag, L":", false);
    if (imageAndTag.size() < 2 || imageAndTag[1].empty())
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ContainerImageTagNotSpecified), nextTraceId));
    }

    for (auto const & volumeRef : volumeRefs_)
    {
        auto error = volumeRef.TryValidate(nextTraceId);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    for (auto const & volume : volumes_)
    {
        auto error = volume.TryValidate(nextTraceId);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    unordered_set<wstring> endpointNames;
    for (auto & endpoint : endpoints_)
    {
        auto error = endpoint.TryValidate(nextTraceId);
        if (!error.IsSuccess())
        {
            return error;
        }

        auto result = endpointNames.insert(endpoint.Name);
        if (!result.second)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(EndpointNameNotUnique), nextTraceId, endpoint.Name));
        }
    }

    for (auto const & setting : settings_)
    {
        auto error = setting.TryValidate(nextTraceId);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return resources_.TryValidate(nextTraceId);
}
