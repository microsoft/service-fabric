//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ReplicaInfoResult)
INITIALIZE_SIZE_ESTIMATION(ReplicasByServiceQueryResult)

ReplicasByServiceQueryResult::ReplicasByServiceQueryResult(
    wstring const &serviceName,
    vector<ReplicaInfoResult> &&replicaInfos)
    : ServiceName(serviceName)
    , ReplicaInfos(move(replicaInfos))
{
}

wstring ReplicasByServiceQueryResult::CreateContinuationToken() const
{
    return ServiceName;
}

void ReplicasByServiceQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ReplicasByServiceQueryResult::ToString() const
{
    return wformatString("ServiceName = {0}, ReplicaInfos.size = {1}", ServiceName, ReplicaInfos.size());
}
