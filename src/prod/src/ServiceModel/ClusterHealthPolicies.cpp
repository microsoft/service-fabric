// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ClusterHealthPolicies::ClusterHealthPolicies()
    : ApplicationHealthPolicies()
    , ClusterHealthPolicyEntry()
{
}

ClusterHealthPolicies::ClusterHealthPolicies(
    ClusterHealthPolicies && other)
    : ApplicationHealthPolicies(move(other.ApplicationHealthPolicies))
    , ClusterHealthPolicyEntry(move(other.ClusterHealthPolicyEntry))
{
}

ClusterHealthPolicies & ClusterHealthPolicies::operator = (ClusterHealthPolicies && other)
{
    if (this != &other)
    {
        this->ClusterHealthPolicyEntry = move(other.ClusterHealthPolicyEntry);
        this->ApplicationHealthPolicies = move(other.ApplicationHealthPolicies);
    }

    return *this;
}
