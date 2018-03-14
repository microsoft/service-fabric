// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

CopyDescription::CopyDescription()
: storeRelativeLocation_()
, version_()
{
}

CopyDescription::CopyDescription(std::wstring const & storeRelativePath, StoreFileVersion const & version)
: storeRelativeLocation_(storeRelativePath)
, version_(version)
{
}

CopyDescription const & CopyDescription::operator = (CopyDescription const & other)
{
    if (this != &other)
    {
        this->storeRelativeLocation_ = other.storeRelativeLocation_;
        this->version_ = other.version_;
    }

    return *this;
}

CopyDescription const & CopyDescription::operator = (CopyDescription && other)
{
    if (this != &other)
    {
        this->storeRelativeLocation_ = move(other.storeRelativeLocation_);
        this->version_ = other.version_;
    }

    return *this;
}

bool CopyDescription::operator == (CopyDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->storeRelativeLocation_, other.storeRelativeLocation_);
    if (!equals) { return equals; }

    equals = this->version_ == other.version_;
    if (!equals) { return equals; }

    return equals;
}

bool CopyDescription::operator != (CopyDescription const & other) const
{
    return !(*this == other);
}

void CopyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring CopyDescription::ToString() const
{
    return wformatString("CopyDescription=[{0}, {1}]", this->storeRelativeLocation_, this->version_);
}

bool CopyDescription::IsEmpty() const
{
    return this->storeRelativeLocation_.empty() || 
        (this->version_ == StoreFileVersion::Default);
}
