// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

StagingLocationInfo::StagingLocationInfo()
    : stagingLocation_()
    , sequenceNumber_(0)
{
}

StagingLocationInfo::StagingLocationInfo(StagingLocationInfo const & other)
{
    this->stagingLocation_ = other.ShareLocation;
    this->sequenceNumber_ = other.SequenceNumber;
}

StagingLocationInfo const & StagingLocationInfo::operator = (StagingLocationInfo const & other)
{
    if (this != &other)
    {
        this->stagingLocation_ = other.ShareLocation;
        this->sequenceNumber_ = other.SequenceNumber;
    }

    return *this;
}

StagingLocationInfo const & StagingLocationInfo::operator = (StagingLocationInfo && other)
{
    if (this != &other)
    {
        this->stagingLocation_ = move(other.ShareLocation);
        this->sequenceNumber_ = other.SequenceNumber;
    }

    return *this;
}

StagingLocationInfo::StagingLocationInfo(wstring const & stagingLocation, uint64 const sequenceNumber)
    : stagingLocation_(stagingLocation)
    , sequenceNumber_(sequenceNumber)
{
}

StagingLocationInfo::~StagingLocationInfo()
{
}

void StagingLocationInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{    
    w.Write("StagingLocationInfo[{0}, {1}]", stagingLocation_, sequenceNumber_);
}

