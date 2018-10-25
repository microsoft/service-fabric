// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace ServiceModel::ModelV2;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("VolumeQueryDescription");

void VolumeQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!volumeNameFilter_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::VolumeResource::VolumeName,
            volumeNameFilter_);
    }

    if (queryPagingDescriptionUPtr_ != nullptr)
    {
        queryPagingDescriptionUPtr_->SetQueryArguments(argMap);
    }
}

Common::ErrorCode VolumeQueryDescription::GetDescriptionFromQueryArgumentMap(
    QueryArgumentMap const & queryArgs)
{
    // Find the VolumeName argument if it exists.
    queryArgs.TryGetValue(QueryResourceProperties::VolumeResource::VolumeName, volumeNameFilter_);

    // This checks for the validity of the PagingQueryDescription as well as retrieving the data.
    QueryPagingDescription pagingDescription;
    auto error = pagingDescription.GetFromArgumentMap(queryArgs);
    if (error.IsSuccess())
    {
        queryPagingDescriptionUPtr_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return error;
}
