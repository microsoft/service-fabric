// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("DeleteServiceDescription");

DeleteServiceDescription::DeleteServiceDescription()
    : serviceName_(),
    isForce_(false)
{
}

DeleteServiceDescription::DeleteServiceDescription(
    Common::NamingUri const & serviceName)
    : serviceName_(serviceName),
    isForce_(false)
{
}

DeleteServiceDescription::DeleteServiceDescription(
    Common::NamingUri const & serviceName,
    bool isForce)
    : serviceName_(serviceName),
    isForce_(isForce)
{
}

DeleteServiceDescription::DeleteServiceDescription(
    DeleteServiceDescription const & other)
    : serviceName_(other.ServiceName),
    isForce_(other.IsForce)
{
}

DeleteServiceDescription DeleteServiceDescription::operator=(
    DeleteServiceDescription const & other)
{
    if (this != &other)
    {
        serviceName_ = other.ServiceName;
        isForce_ = other.IsForce;
    }

    return *this;
}

Common::ErrorCode DeleteServiceDescription::FromPublicApi(FABRIC_DELETE_SERVICE_DESCRIPTION const & svcDeleteDesc)
{
    ErrorCode err = NamingUri::TryParse(svcDeleteDesc.ServiceName, "ServiceName", serviceName_);

    if (!err.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "DeleteServiceDescription::FromPublicApi failed to parse ServiceName = {0} error = {1} errormessage = {2}.", svcDeleteDesc.ServiceName, err, err.Message);
        return err;
    }

    isForce_ = svcDeleteDesc.ForceDelete ? true: false;

    return ErrorCodeValue::Success;
}

string DeleteServiceDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "(serviceName={0} isForce={1})";
    size_t index = 0;

    traceEvent.AddEventField<Uri>(format, name + ".serviceName", index);
    traceEvent.AddEventField<bool>(format, name + ".isForce", index);
    return format;
}

void DeleteServiceDescription::FillEventData(TraceEventContext & context) const
{
    context.Write<NamingUri>(serviceName_);
    context.Write<bool>(isForce_);
}

void DeleteServiceDescription::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("DeleteServiceDescription({0}: {1})", serviceName_, isForce_);
}
