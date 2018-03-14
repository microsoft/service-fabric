// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("DeleteApplicationDescription");

DeleteApplicationDescription::DeleteApplicationDescription()
    : applicationName_()
    , isForce_(false)
{
}

DeleteApplicationDescription::DeleteApplicationDescription(
    Common::NamingUri const & applicationName)
    : applicationName_(applicationName),
    isForce_(false)
{
}

DeleteApplicationDescription::DeleteApplicationDescription(
    Common::NamingUri const & applicationName,
    bool isForce)
    : applicationName_(applicationName),
    isForce_(isForce)
{
}

DeleteApplicationDescription::DeleteApplicationDescription(
    DeleteApplicationDescription const & other)
    : applicationName_(other.ApplicationName)
    , isForce_(other.IsForce)
{
}

DeleteApplicationDescription DeleteApplicationDescription::operator=(
    DeleteApplicationDescription const & other)
{
    if (this != &other)
    {
        applicationName_ = other.ApplicationName;
        isForce_ = other.IsForce;
    }

    return *this;
}

Common::ErrorCode DeleteApplicationDescription::FromPublicApi(FABRIC_DELETE_APPLICATION_DESCRIPTION const & appDeleteDesc)
{
    ErrorCode err = NamingUri::TryParse(appDeleteDesc.ApplicationName, "ApplicationName", applicationName_);

    if (!err.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "DeleteApplicationDescription::FromPublicApi failed to parse ApplicationName = {0}, error = {1} errormessage = {2}.", appDeleteDesc.ApplicationName, err, err.Message);
        return err;
    }

    isForce_ = appDeleteDesc.ForceDelete ? true: false;

    return ErrorCodeValue::Success;
}

string DeleteApplicationDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "(applicationName={0} isForce={1})";
    size_t index = 0;

    traceEvent.AddEventField<Uri>(format, name + ".applicationName", index);
    traceEvent.AddEventField<bool>(format, name + ".isForce", index);
    return format;
}

void DeleteApplicationDescription::FillEventData(TraceEventContext & context) const
{
    context.Write<NamingUri>(applicationName_);
    context.Write<bool>(isForce_);
}

void DeleteApplicationDescription::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("DeleteApplicationDescription({0}: {1})", applicationName_, isForce_);
}
