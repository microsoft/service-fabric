// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PackageSharingPolicyList::PackageSharingPolicyList()
    : packageSharingPolicies_()
{
}

PackageSharingPolicyList::PackageSharingPolicyList(
    vector<PackageSharingPolicyQueryObject> const & packageSharingPolicies)
    : packageSharingPolicies_(packageSharingPolicies)
{
}

PackageSharingPolicyList::PackageSharingPolicyList(
    PackageSharingPolicyList && other) 
    : packageSharingPolicies_(move(other.packageSharingPolicies_))
{
}

PackageSharingPolicyList const & PackageSharingPolicyList::operator= (PackageSharingPolicyList && other)
{
    if(this != &other)
    {
        this->packageSharingPolicies_ = move(other.packageSharingPolicies_);
    }
    return *this;
}

void PackageSharingPolicyList::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("PackageSharingPolicyList { ");
    for(auto iter = packageSharingPolicies_.begin(); iter != packageSharingPolicies_.end(); iter++)
    {
        w.Write("PackageSharingPolicy = {0}", *iter);
    }
    w.Write("}");
}
