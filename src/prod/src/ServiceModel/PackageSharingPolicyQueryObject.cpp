// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PackageSharingPolicyQueryObject::PackageSharingPolicyQueryObject()
    : packageName_(),
    sharingScope_()
{
}

PackageSharingPolicyQueryObject::PackageSharingPolicyQueryObject(
    wstring const & packageName,
    ServicePackageSharingType::Enum sharingScope)
    : packageName_(packageName),
    sharingScope_(sharingScope)
{
}

PackageSharingPolicyQueryObject::PackageSharingPolicyQueryObject(
    PackageSharingPolicyQueryObject && other) 
    : packageName_(move(other.packageName_)),
    sharingScope_(move(other.sharingScope_))
{
}

PackageSharingPolicyQueryObject const & PackageSharingPolicyQueryObject::operator= (PackageSharingPolicyQueryObject && other)
{
    if(this != &other)
    {
        this->packageName_ = move(other.packageName_);
        this->sharingScope_ = move(other.sharingScope_);
    }
    return *this;
}

void PackageSharingPolicyQueryObject::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("PackageSharingPolicyQueryObject { ");
    w.Write("SharedPackageName = {0}", packageName_);
    w.Write("SharingScope = {0}", sharingScope_);
    w.Write("}");
}
