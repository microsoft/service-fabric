//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StoreDataSingleInstanceApplicationInstance::StoreDataSingleInstanceApplicationInstance(
    wstring const & deploymentName,
    ServiceModelVersion const & version)
    : DeploymentName(deploymentName)
    , TypeVersion(version)
    , ApplicationDescription()
{
}

StoreDataSingleInstanceApplicationInstance::StoreDataSingleInstanceApplicationInstance(
    wstring const & deploymentName,
    ServiceModelVersion const & version,
    ModelV2::ApplicationDescription const & description)
    : DeploymentName(deploymentName)
    , TypeVersion(version)
    , ApplicationDescription(description)
{
}

wstring const & StoreDataSingleInstanceApplicationInstance::get_Type() const
{
    return Constants::StoreType_SingleInstanceApplicationInstance;
}

void StoreDataSingleInstanceApplicationInstance::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "StoreDataSingleInstanceApplicationInstance({0}:{1}, Description: {2})",
        DeploymentName,
        TypeVersion,
        ApplicationDescription);
}

wstring StoreDataSingleInstanceApplicationInstance::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}{2}",
        DeploymentName,
        Constants::TokenDelimeter,
        TypeVersion);
    return temp;
}

wstring StoreDataSingleInstanceApplicationInstance::GetDeploymentNameKeyPrefix() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        DeploymentName,
        Constants::TokenDelimeter);
    return temp;
}
