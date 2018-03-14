// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

VersionedServicePackageIdentifier::VersionedServicePackageIdentifier()
    : id_(),
    version_()
{
}

VersionedServicePackageIdentifier::VersionedServicePackageIdentifier(
    ServicePackageIdentifier const & id, 
    ServicePackageVersion const & version)
    : id_(id),
    version_(version)
{
}

VersionedServicePackageIdentifier::VersionedServicePackageIdentifier(VersionedServicePackageIdentifier const & other)
    : id_(other.id_),
    version_(other.version_)
{
}

VersionedServicePackageIdentifier::VersionedServicePackageIdentifier(VersionedServicePackageIdentifier && other)
    : id_(move(other.id_)),
    version_(move(other.version_))
{
}

VersionedServicePackageIdentifier const & VersionedServicePackageIdentifier::operator = (VersionedServicePackageIdentifier const & other)
{
    if (this != &other)
    {
        this->id_ = other.id_;
        this->version_ = other.version_;
    }

    return *this;
}

VersionedServicePackageIdentifier const & VersionedServicePackageIdentifier::operator = (VersionedServicePackageIdentifier && other)
{
    if (this != &other)
    {
        this->id_ = move(other.id_);
        this->version_ = move(other.version_);
    }

    return *this;
}

bool VersionedServicePackageIdentifier::operator == (VersionedServicePackageIdentifier const & other) const
{
    bool equals = true;

    equals = (this->id_ == other.id_);
    if (!equals) { return false; }

    equals = (this->version_ == other.version_);
    if (!equals) { return false; }

    return equals;
}

bool VersionedServicePackageIdentifier::operator != (VersionedServicePackageIdentifier const & other) const
{
    return !(*this == other);
}

int VersionedServicePackageIdentifier::compare(VersionedServicePackageIdentifier const & other) const
{
    int comparision = 0;
    
    comparision = this->id_.compare(other.id_);
    if (comparision != 0) { return comparision; }

    comparision = this->version_.compare(other.version_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool VersionedServicePackageIdentifier::operator < (VersionedServicePackageIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void VersionedServicePackageIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring VersionedServicePackageIdentifier::ToString() const 
{
    std::wstring str;
    Common::StringWriter writer(str);
    writer.Write("{");
    writer.Write("{0},{1}", id_, version_);
    writer.Write("}");
    writer.Flush();
    
    return str; 
}
