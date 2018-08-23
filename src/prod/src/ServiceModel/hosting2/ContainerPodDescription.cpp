//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

void PodPortMapping::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("PodPortMapping { ");
    w.Write("HostIp = {0}", HostIp);
    w.Write("Protocol = {0}", (int)Proto);
    w.Write("ContainerPort = {0}", ContainerPort);
    w.Write("HostPort = {0}", HostPort);
    w.Write("}");
}

ContainerPodDescription::ContainerPodDescription()
    : hostName_(),
    isolationMode_(ContainerIsolationMode::Enum::process),
    portMappings_()
{
}

ContainerPodDescription::ContainerPodDescription(
    std::wstring const & hostName,
    ContainerIsolationMode::Enum const & isolationMode,
    std::vector<PodPortMapping> const & portMappings)
    : hostName_(hostName),
    isolationMode_(isolationMode),
    portMappings_(portMappings)
{
}

ContainerPodDescription::ContainerPodDescription(ContainerPodDescription const & other)
    : hostName_(other.hostName_),
    isolationMode_(other.isolationMode_),
    portMappings_(other.portMappings_)
{
}

ContainerPodDescription::ContainerPodDescription(ContainerPodDescription && other)
    : hostName_(move(other.hostName_)),
    isolationMode_(other.isolationMode_),
    portMappings_(move(other.portMappings_))
{
}

ContainerPodDescription::~ContainerPodDescription()
{
}

ContainerPodDescription const & ContainerPodDescription::operator=(ContainerPodDescription const & other)
{
    if(this != &other)
    {
        this->hostName_ = other.hostName_;
        this->isolationMode_ = other.isolationMode_;
        this->portMappings_ = other.portMappings_;
    }
    return *this;
}

ContainerPodDescription const & ContainerPodDescription::operator=(ContainerPodDescription && other)
{
    if(this != &other)
    {
        this->hostName_ = move(other.hostName_);
        this->isolationMode_ = move(other.isolationMode_);
        this->portMappings_ = move(other.portMappings_);
    }
    return *this;
}

void ContainerPodDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerPodDescription { ");
    w.Write("HostName = {0}", HostName);
    w.Write("IsolationMode = {0}", (int)IsolationMode);

    w.Write("PodPortMappings { ");
    for (auto pm : portMappings_)
    {
        w.Write("{0}", PortMappings);
    }
    w.Write("}");

    w.Write("}");
}
