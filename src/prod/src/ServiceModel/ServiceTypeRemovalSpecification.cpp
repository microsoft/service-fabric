// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceTypeRemovalSpecification::ServiceTypeRemovalSpecification()
    : packageName_(),
    packageRolloutVersion_(),
    typeNames_()
{
}

ServiceTypeRemovalSpecification::ServiceTypeRemovalSpecification(
    wstring const & packageName, 
    RolloutVersion const & packageRolloutVersion)
    : packageName_(packageName),
    packageRolloutVersion_(packageRolloutVersion),
    typeNames_()
{
}

ServiceTypeRemovalSpecification::ServiceTypeRemovalSpecification(ServiceTypeRemovalSpecification && other)
    : packageName_(forward<wstring>(other.packageName_)),
    packageRolloutVersion_(forward<RolloutVersion>(other.packageRolloutVersion_)),
    typeNames_(forward<set<wstring>>(other.typeNames_))
{
}

ServiceTypeRemovalSpecification const & ServiceTypeRemovalSpecification::operator = (ServiceTypeRemovalSpecification && other)
{
    if (this != &other)
    {
        this->packageName_ = forward<wstring>(other.packageName_);
        this->packageRolloutVersion_ = forward<RolloutVersion>(other.packageRolloutVersion_);
        this->typeNames_ = forward<set<wstring>>(other.typeNames_);
    }

    return *this;
}

void ServiceTypeRemovalSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write("PackageName={0},RolloutVersion={1},TypeNames={2}", packageName_, packageRolloutVersion_, typeNames_);
}

void ServiceTypeRemovalSpecification::AddServiceTypeName(std::wstring const & typeName)
{
    this->typeNames_.insert(typeName);
}

bool ServiceTypeRemovalSpecification::HasServiceTypeName(std::wstring const & typeName) const
{
    return typeNames_.find(typeName) != typeNames_.end();
}
