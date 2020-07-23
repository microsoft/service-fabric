// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ContainerServiceProperties)
INITIALIZE_SIZE_ESTIMATION(ContainerServiceDescription)

bool ContainerServiceProperties::operator==(ContainerServiceProperties const & other) const
{
    if (ServicePropertiesBase::operator==(other)
        && replicaCount_ == other.replicaCount_
        && description_ == other.description_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ContainerServiceProperties::operator!=(ContainerServiceProperties const & other) const
{
    return !(*this == other);
}

ErrorCode ContainerServiceProperties::TryValidate(wstring const &traceId) const
{
    if (codePackages_.size() == 0)
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(CodePackageNotSpecified), traceId));
    }

    ErrorCode error;
    unordered_set<wstring> codePackageNames;
    unordered_set<wstring> endpointNames;
    for (auto const &codePackage : codePackages_)
    {
        error = codePackage.TryValidate(traceId);
        if (!error.IsSuccess())
        {
            return error;
        }

        auto result = codePackageNames.insert(codePackage.Name);
        std::vector<ServiceModel::ModelV2::ContainerEndpointDescription> endpointRefs = codePackage.EndpointRefs;
        for (const auto & endpoint : endpointRefs)
        {
            result = endpointNames.insert(endpoint.Name);
        }

        if (!result.second)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(CodePackageNameNotUnique), traceId, codePackage.Name));
        }
    }

    unordered_set<wstring> endpointsReferencedByNetwork;
    for (auto const &networkRef : networkRefs_)
    {
        std::vector<ServiceModel::ModelV2::EndpointRef> endpointRefs = networkRef.Endpoints;
        for (auto & endpointRef : endpointRefs)
        {
            auto result = endpointsReferencedByNetwork.insert(endpointRef.Name);
        }
    }

    if (endpointNames.size() != endpointsReferencedByNetwork.size())
    {
        for(auto const & endpoint : endpointNames)
        {
            unordered_set<wstring>::const_iterator got = endpointsReferencedByNetwork.find(endpoint);
            if (got == endpointsReferencedByNetwork.end())
            {
                return ErrorCode(ErrorCodeValue::EndpointNotReferenced, wformatString(GET_MODELV2_RC(EndpointNotReferenced), traceId, endpoint));
            }
        }
    }

    for (auto const &autoScalingPolicy : autoScalingPolicies_)
    {
        error = autoScalingPolicy.TryValidate(traceId);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

bool ContainerServiceProperties::CanUpgrade(ContainerServiceProperties const & other) const
{
    // replicaCount_ is allowed to be changed for upgrade.
    if (description_ != other.description_
        || !ServicePropertiesBase::CanUpgrade(other))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool ContainerServiceDescription::operator==(ContainerServiceDescription const & other) const
{
    if (ResourceDescription::operator==(other)
        && this->Properties == other.Properties)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ContainerServiceDescription::operator!=(ContainerServiceDescription const & other) const
{
    return !(*this == other);
}

ErrorCode ContainerServiceDescription::TryValidate(wstring const & traceId) const
{
    ErrorCode error = __super::TryValidate(traceId);
    if (!error.IsSuccess())
    {
        return move(error);
    }

    return this->Properties.TryValidate(GetNextTraceId(traceId, ServiceName));
}

bool ContainerServiceDescription::CanUpgrade(ContainerServiceDescription const & other) const
{
    if (this->ServiceName != other.ServiceName
        || !this->Properties.CanUpgrade(other.Properties))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void ContainerServiceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        ServiceName);
}
