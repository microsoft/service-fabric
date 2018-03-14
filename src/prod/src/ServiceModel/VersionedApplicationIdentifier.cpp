// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

VersionedApplicationIdentifier::VersionedApplicationIdentifier()
    : id_(),
    version_()
{
}

VersionedApplicationIdentifier::VersionedApplicationIdentifier(
    ApplicationIdentifier const & id, 
    ApplicationVersion const & version)
    : id_(id),
    version_(version)
{
}

VersionedApplicationIdentifier::VersionedApplicationIdentifier(VersionedApplicationIdentifier const & other)
    : id_(other.id_),
    version_(other.version_)
{
}

VersionedApplicationIdentifier::VersionedApplicationIdentifier(VersionedApplicationIdentifier && other)
    : id_(move(other.id_)),
    version_(move(other.version_))
{
}

VersionedApplicationIdentifier const & VersionedApplicationIdentifier::operator = (VersionedApplicationIdentifier const & other)
{
    if (this != &other)
    {
        this->id_ = other.id_;
        this->version_ = other.version_;
    }

    return *this;
}

VersionedApplicationIdentifier const & VersionedApplicationIdentifier::operator = (VersionedApplicationIdentifier && other)
{
    if (this != &other)
    {
        this->id_ = move(other.id_);
        this->version_ = move(other.version_);
    }

    return *this;
}

bool VersionedApplicationIdentifier::operator == (VersionedApplicationIdentifier const & other) const
{
    bool equals = true;

    equals = (this->id_ == other.id_);
    if (!equals) { return false; }

    equals = (this->version_ == other.version_);
    if (!equals) { return false; }

    return equals;
}

bool VersionedApplicationIdentifier::operator != (VersionedApplicationIdentifier const & other) const
{
    return !(*this == other);
}

int VersionedApplicationIdentifier::compare(VersionedApplicationIdentifier const & other) const
{
    int comparision = 0;
    
    comparision = this->id_.compare(other.id_);
    if (comparision != 0) { return comparision; }

    comparision = this->version_.compare(other.version_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool VersionedApplicationIdentifier::operator < (VersionedApplicationIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void VersionedApplicationIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring VersionedApplicationIdentifier::ToString() const 
{
    std::wstring str;
    Common::StringWriter writer(str);
    writer.Write("{");
    writer.Write("{0},{1}", id_, version_);
    writer.Write("}");
    writer.Flush();
    
    return str; 
}
