//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeleteSingleInstanceDeploymentDescription::DeleteSingleInstanceDeploymentDescription(wstring const & deploymentName)
    : deploymentName_(deploymentName)
{
}

DeleteSingleInstanceDeploymentDescription DeleteSingleInstanceDeploymentDescription::operator=(
    DeleteSingleInstanceDeploymentDescription const & other)
{
    if (this != &other)
    {
        deploymentName_ = other.DeploymentName;
    }

    return *this;
}

string DeleteSingleInstanceDeploymentDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "(deploymentName={0}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".deploymentName", index);
    return format;
}

void DeleteSingleInstanceDeploymentDescription::FillEventData(TraceEventContext & context) const
{
    context.Write<wstring>(deploymentName_);
}

void DeleteSingleInstanceDeploymentDescription::WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const
{
    writer.Write("DeleteSingleInstanceDeploymentDescription({0})", deploymentName_);
}
