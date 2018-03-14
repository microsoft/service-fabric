// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

INITIALIZE_SIZE_ESTIMATION(StoreFileVersion)

StoreFileVersion StoreFileVersion::Default = StoreFileVersion();

StoreFileVersion::StoreFileVersion()
    : epochDataLossNumber_(0)
    , epochConfigurationNumber_(-1)
    , versionNumber_(0)    
{
}

StoreFileVersion::StoreFileVersion(
    int64 const epochDataLossNumber, 
    int64 const epochConfigurationNumber,
    uint64 const versionNumber)
    : epochDataLossNumber_(epochDataLossNumber)
    , epochConfigurationNumber_(epochConfigurationNumber)
    , versionNumber_(versionNumber)
{
}

StoreFileVersion const & StoreFileVersion::operator = (StoreFileVersion const & other)
{
    if (this != &other)
    {
        this->epochDataLossNumber_ = other.epochDataLossNumber_;
        this->epochConfigurationNumber_ = other.epochConfigurationNumber_;
        this->versionNumber_ = other.versionNumber_;
    }

    return *this;
}

StoreFileVersion const & StoreFileVersion::operator = (StoreFileVersion && other)
{
    if (this != &other)
    {
        this->epochDataLossNumber_ = other.epochDataLossNumber_;
        this->epochConfigurationNumber_ = other.epochConfigurationNumber_;
        this->versionNumber_ = other.versionNumber_;
    }

    return *this;
}

bool StoreFileVersion::operator == (StoreFileVersion const & other) const
{
    bool equals = true;

    equals = this->epochDataLossNumber_ == other.epochDataLossNumber_;
    if (!equals) { return equals; }

    equals = this->epochConfigurationNumber_ == other.epochConfigurationNumber_;
    if(!equals) { return equals; }

    equals = this->versionNumber_ == other.versionNumber_;
    if (!equals) { return equals; }

    return equals;
}

bool StoreFileVersion::operator != (StoreFileVersion const & other) const
{
    return !(*this == other);
}

void StoreFileVersion::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write(ToString());
}

wstring StoreFileVersion::ToString() const
{
    if(this->epochConfigurationNumber_ == -1)
    {
        return wformatString("{0}_{1}", this->epochDataLossNumber_, this->versionNumber_);
    }
    else
    {
        return wformatString("{0}_{1}_{2}", this->epochDataLossNumber_, this->epochConfigurationNumber_, this->versionNumber_);
    }
}

bool StoreFileVersion::IsStoreFileVersion(std::wstring const & stringValue)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(stringValue, tokens, L"_", true);

    int64 value;
    uint64 unsignedValue;
    if(tokens.size() == 2)
    {        
        if(!TryParseInt64(tokens[0], value)) { return false; }
        if(!TryParseUInt64(tokens[1], unsignedValue)) { return false; }

        return true;
    }
    else if(tokens.size() == 3)
    {
        if(!TryParseInt64(tokens[0], value)) { return false; }
        if(!TryParseInt64(tokens[1], value)) { return false; }
        if(!TryParseUInt64(tokens[2], unsignedValue)) { return false; }

        return true;
    }
    else
    {
        return false;
    }    
}
