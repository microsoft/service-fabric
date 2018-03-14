// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("StoreDataServiceManifest");

StoreDataServiceManifest::StoreDataServiceManifest()
    : applicationTypeName_()
    , applicationTypeVersion_()
    , serviceManifests_()
{
}

StoreDataServiceManifest::StoreDataServiceManifest(
    ServiceModelTypeName const & applicationTypeName,
    ServiceModelVersion const & applicationTypeVersion)
    : applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
    , serviceManifests_()
{
}

StoreDataServiceManifest::StoreDataServiceManifest(
    ServiceModelTypeName const & applicationTypeName,
    ServiceModelVersion const & applicationTypeVersion,
    vector<ServiceModelServiceManifestDescription> && serviceManifests)
    : applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)    
    , serviceManifests_(move(serviceManifests))
{
}

void StoreDataServiceManifest::TrimDuplicateManifestsAndPackages(vector<StoreDataServiceManifest> const & provisioned)
{
    // <Name, Version> of manifests to remove from serviceManifests_ because it's still
    // being used by another version of this same application type
    //
    vector<pair<wstring, wstring>> itemsToErase;

    for (auto it = provisioned.begin(); it != provisioned.end(); ++it)
    {
        if (it->ApplicationTypeName != applicationTypeName_ || it->ApplicationTypeVersion == applicationTypeVersion_)
        {
            continue;
        }

        WriteInfo(
            TraceComponent, 
            "{0} trim ({1},{2}) : {3} against ({4}, {5}) : {6}",
            this->TraceId,
            applicationTypeName_,
            applicationTypeVersion_,
            serviceManifests_,
            it->ApplicationTypeName,
            it->ApplicationTypeVersion,
            it->serviceManifests_);

        for (auto & thisManifest : serviceManifests_)
        {
            for (auto const & otherManifest : it->serviceManifests_)
            {
                if (thisManifest.Name == otherManifest.Name)
                {
                    if (thisManifest.Version == otherManifest.Version)
                    {
                        itemsToErase.push_back(make_pair(thisManifest.Name, thisManifest.Version));
                        break;
                    }
                    else
                    {
                        // different version of the service manifest, but still need to check if individual packages are being shared
                        thisManifest.TrimDuplicatePackages(otherManifest);
                    }
                }
            }
        }
    } // for other provisioned application types

    for (auto const & toErase : itemsToErase)
    {
        serviceManifests_.erase(remove_if(serviceManifests_.begin(), serviceManifests_.end(),
            [&toErase](ServiceModelServiceManifestDescription const & item)
            { 
                return (item.Name == toErase.first && item.Version == toErase.second); 
            }),
            serviceManifests_.end());
    }
}

std::wstring StoreDataServiceManifest::GetTypeNameKeyPrefix() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        applicationTypeName_,
        Constants::TokenDelimeter);
    return temp;
}


std::wstring const & StoreDataServiceManifest::get_Type() const
{
    return Constants::StoreType_ServiceManifest;
}

std::wstring StoreDataServiceManifest::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        GetTypeNameKeyPrefix(),
        applicationTypeVersion_);
    return temp;
}

void StoreDataServiceManifest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("StoreDataApplicationManifest[{0}, {1}]", applicationTypeName_, applicationTypeVersion_);
}
