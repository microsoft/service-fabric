// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationUpdateContext");

RolloutContextType::Enum const ApplicationUpdateContextType(RolloutContextType::ApplicationUpdate);

ApplicationUpdateContext::ApplicationUpdateContext(
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest,
    Common::NamingUri const & applicationName,
    ServiceModelApplicationId const & applicationId,
    Reliability::ApplicationCapacityDescription const & currentCapacity,
    Reliability::ApplicationCapacityDescription const & updatedCapacity)
    : RolloutContext(ApplicationUpdateContextType, replica, clientRequest)
    , applicationName_(applicationName)
    , applicationId_(applicationId)
    , currentCapacities_(currentCapacity)
    , updatedCapacities_(updatedCapacity)
{
}


ApplicationUpdateContext::ApplicationUpdateContext(ApplicationUpdateContext && other)
    : RolloutContext(move(other))
    , applicationName_(move(other.applicationName_))
    , applicationId_(move(other.applicationId_))
    , currentCapacities_(move(other.currentCapacities_))
    , updatedCapacities_(move(other.updatedCapacities_))
{
}

ApplicationUpdateContext::ApplicationUpdateContext(
    Common::NamingUri const & appName)
    : RolloutContext(ApplicationUpdateContextType)
    , applicationName_(appName)
    , applicationId_(0)
    , currentCapacities_()
    , updatedCapacities_()
{
}

ApplicationUpdateContext & ApplicationUpdateContext::operator=(
    ApplicationUpdateContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        this->applicationName_ = move(other.applicationName_);
        this->applicationId_ = move(other.applicationId_);
        this->currentCapacities_ = move(other.currentCapacities_);
        this->updatedCapacities_ = move(other.updatedCapacities_);
    }

    return *this;
}

std::wstring const & ApplicationUpdateContext::get_Type() const
{
    return Constants::StoreType_ApplicationUpdateContext;
}

std::wstring ApplicationUpdateContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", applicationName_);
    return temp;
}

void ApplicationUpdateContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "ApplicationUpdateContext({0})[name={1}, applicationId={2}, currentCapacities=[{3}], updatedCapacities=[{4}]]",
        this->Status, 
        applicationName_,
        applicationId_,
        currentCapacities_,
        updatedCapacities_);
}
