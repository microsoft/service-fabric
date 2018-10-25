//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ApplicationProperties)
INITIALIZE_SIZE_ESTIMATION(ApplicationDescription)

bool ApplicationProperties::operator==(ApplicationProperties const & other) const
{
    if (description_ == other.description_
        && Services == other.Services)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode ApplicationProperties::TryValidate(wstring const &traceId) const
{
    ErrorCode error;
    unordered_set<wstring> serviceNames;

    //
    // TODO: Remove this validation when we support 'dynamic' services under application.
    //
    if (Services.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ServiceNotSpecified), traceId));
    }

    for (auto &service : Services)
    {
        error = service.TryValidate(traceId);
        if (!error.IsSuccess())
        {
            return error;
        }

        auto result = serviceNames.insert(service.ServiceName);
        if (!result.second)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ServiceNameNotUnique), traceId, service.ServiceName));
        }
    }

    return error;
}

bool ApplicationDescription::operator==(ApplicationDescription const & other) const
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

ErrorCode ApplicationDescription::TryValidate(wstring const &traceId) const
{
    ErrorCode error = __super::TryValidate(traceId);
    if (!error.IsSuccess())
    {
        return move(error);
    }

    return this->Properties.TryValidate(GetNextTraceId(traceId, Name));
}

bool ApplicationDescription::CanUpgrade(ApplicationDescription & other)
{
    if (Name == other.Name
        && this->Description == other.Description)
    {
        bool canUpgrade = false;
        for (auto i = 0; i < Services.size(); ++i)
        {
            for (auto j = 0; j < other.Services.size(); ++j)
            {
                if (this->Services[i].ServiceName == other.Services[j].ServiceName)
                {
                    if (Services[i].CanUpgrade(other.Services[j]))
                    {
                        canUpgrade = true;
                    }
                    else if (Services[i] != other.Services[j])
                    {
                        return false;
                    }
                }
            }
        }

        return canUpgrade;
    }
    else
    {
        return false;
    }
}

void ApplicationDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}
