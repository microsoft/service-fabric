// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceContext("ReplicasResourceQueryDescription");

ReplicasResourceQueryDescription::ReplicasResourceQueryDescription(
    NamingUri && applicationUri,
    NamingUri && serviceName)
    : applicationUri_(move(applicationUri))
    , serviceName_(move(serviceName))
    , replicaId_(-1)
    , queryPagingDescription_()
{
}

ReplicasResourceQueryDescription::ReplicasResourceQueryDescription(
    NamingUri && applicationUri,
    NamingUri && serviceName,
    int64 replicaId)
    : applicationUri_(move(applicationUri))
    , serviceName_(move(serviceName))
    , replicaId_(replicaId)
    , queryPagingDescription_()
{
}
