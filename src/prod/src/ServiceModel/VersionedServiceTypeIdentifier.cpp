// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

VersionedServiceTypeIdentifier::VersionedServiceTypeIdentifier()
    : id_(),
    servicePackageVersionInstance_()
{
}

VersionedServiceTypeIdentifier::VersionedServiceTypeIdentifier(
    ServiceTypeIdentifier const & id, 
    ServicePackageVersionInstance const & versionInstance)
    : id_(id),
    servicePackageVersionInstance_(versionInstance)
{
}

VersionedServiceTypeIdentifier::VersionedServiceTypeIdentifier(VersionedServiceTypeIdentifier const & other)
    : id_(other.id_),
    servicePackageVersionInstance_(other.servicePackageVersionInstance_)
{
}

VersionedServiceTypeIdentifier::VersionedServiceTypeIdentifier(VersionedServiceTypeIdentifier && other)
    : id_(move(other.id_)),
    servicePackageVersionInstance_(move(other.servicePackageVersionInstance_))
{
}

VersionedServiceTypeIdentifier const & VersionedServiceTypeIdentifier::operator = (VersionedServiceTypeIdentifier const & other)
{
    if (this != &other)
    {
        this->id_ = other.id_;
        this->servicePackageVersionInstance_ = other.servicePackageVersionInstance_;
    }

    return *this;
}

VersionedServiceTypeIdentifier const & VersionedServiceTypeIdentifier::operator = (VersionedServiceTypeIdentifier && other)
{
    if (this != &other)
    {
        this->id_ = move(other.id_);
        this->servicePackageVersionInstance_ = move(other.servicePackageVersionInstance_);
    }

    return *this;
}

bool VersionedServiceTypeIdentifier::operator == (VersionedServiceTypeIdentifier const & other) const
{
    bool equals = true;

    equals = (this->id_ == other.id_);
    if (!equals) { return false; }

    equals = (this->servicePackageVersionInstance_ == other.servicePackageVersionInstance_);
    if (!equals) { return false; }

    return equals;
}

bool VersionedServiceTypeIdentifier::operator != (VersionedServiceTypeIdentifier const & other) const
{
    return !(*this == other);
}

int VersionedServiceTypeIdentifier::compare(VersionedServiceTypeIdentifier const & other) const
{
    int comparision = 0;
    
    comparision = this->id_.compare(other.id_);
    if (comparision != 0) { return comparision; }

    comparision = this->servicePackageVersionInstance_.compare(other.servicePackageVersionInstance_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool VersionedServiceTypeIdentifier::operator < (VersionedServiceTypeIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void VersionedServiceTypeIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring VersionedServiceTypeIdentifier::ToString() const 
{
    std::wstring str;
    Common::StringWriter writer(str);
    writer.Write("{");
    writer.Write("{0},{1}", id_, servicePackageVersionInstance_);
    writer.Write("}");
    writer.Flush();
    
    return str; 
}
