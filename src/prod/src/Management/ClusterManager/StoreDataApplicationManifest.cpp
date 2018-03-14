// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace ServiceModel;

StoreDataApplicationManifest::StoreDataApplicationManifest()
    : typeName_()
    , typeVersion_()
    , applicationManifest_()
    , healthPolicy_()
    , applicationParameters_()
{
}

StoreDataApplicationManifest::StoreDataApplicationManifest(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion)
    : typeName_(typeName)
    , typeVersion_(typeVersion)
    , applicationManifest_()
    , healthPolicy_()
    , applicationParameters_()
{
}

StoreDataApplicationManifest::StoreDataApplicationManifest(
    StoreDataApplicationManifest && other)
    : typeName_(move(other.typeName_))
    , typeVersion_(move(other.typeVersion_))
    , applicationManifest_(move(other.applicationManifest_))
    , healthPolicy_(move(other.healthPolicy_))
    , applicationParameters_(move(other.applicationParameters_))
{
}

StoreDataApplicationManifest::StoreDataApplicationManifest(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    wstring && applicationManifest,
    ServiceModel::ApplicationHealthPolicy && healthPolicy,
    map<wstring, wstring> && applicationParameters)
    : typeName_(typeName)
    , typeVersion_(typeVersion)    
    , applicationManifest_(move(applicationManifest))
    , healthPolicy_(move(healthPolicy))
    , applicationParameters_(move(applicationParameters))
{
}

wstring const & StoreDataApplicationManifest::get_Type() const
{
    return Constants::StoreType_ApplicationManifest;
}

wstring StoreDataApplicationManifest::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}{2}",
        typeName_,
        Constants::TokenDelimeter,
        typeVersion_);
    return temp;
}

void StoreDataApplicationManifest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    wstring applicationParameters = L"";
    for (auto itApplicationParameter = applicationParameters_.begin(); itApplicationParameter != applicationParameters_.end(); ++ itApplicationParameter)
    {
        applicationParameters.append(wformatString("{0}={1};", itApplicationParameter->first, itApplicationParameter->second));
    }

    w.Write("StoreDataApplicationManifest[{0}, {1}, {2}]", typeName_, typeVersion_, applicationParameters);
}

map<wstring, wstring> StoreDataApplicationManifest::GetMergedApplicationParameters(
    map<wstring, wstring> const & parameters) const
{
    auto mergedParameters = parameters;

    for (auto it=applicationParameters_.begin(); it!=applicationParameters_.end(); ++it)
    {
        auto find_it = mergedParameters.find(it->first);
        if (find_it == mergedParameters.end())
        {
            mergedParameters.insert(*it);
        }
    }

    return move(mergedParameters);
}
