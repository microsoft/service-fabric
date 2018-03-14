// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceGroupMemberTypeQueryResult::ServiceGroupMemberTypeQueryResult() 
    : serviceGroupMembers_()
    , serviceManifestVersion_()
    , serviceManifestName_()
{
}

ServiceGroupMemberTypeQueryResult::ServiceGroupMemberTypeQueryResult(
    vector<ServiceGroupTypeMemberDescription> const & serviceGroupMemberTypeDescription,
    wstring const & serviceManifestVersion,
    wstring const & serviceManifestName)
    : serviceGroupMembers_(serviceGroupMemberTypeDescription)
    , serviceManifestVersion_(serviceManifestVersion)
    , serviceManifestName_(serviceManifestName)
{
}

void ServiceGroupMemberTypeQueryResult::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM & publicServiceGroupMemberTypeQueryResult) const
{
    publicServiceGroupMemberTypeQueryResult.ServiceManifestVersion = heap.AddString(serviceManifestVersion_);

    auto memberList = heap.AddItem<FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST>();
    ULONG const memberListSize = static_cast<const ULONG>(serviceGroupMembers_.size());
    memberList->Count = memberListSize;

    if (memberListSize > 0)
    {
        auto members = heap.AddArray<FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION>(memberListSize);
        for (size_t i = 0; i < serviceGroupMembers_.size(); i++)
        {
            serviceGroupMembers_[i].ToPublicApi(heap, members[i]);
        }

        memberList->Items = members.GetRawArray();
    }
    else
    {
        memberList->Items = NULL;
    }

    publicServiceGroupMemberTypeQueryResult.ServiceGroupMemberTypeDescription = memberList.GetRawPointer();

    publicServiceGroupMemberTypeQueryResult.ServiceManifestName = heap.AddString(serviceManifestName_);
}
