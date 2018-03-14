// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

PartialCopyProgressData::PartialCopyProgressData()
    : lastStartLsn_(0) 
{ 
}

PartialCopyProgressData::PartialCopyProgressData(FABRIC_SEQUENCE_NUMBER lsn) 
    : lastStartLsn_(lsn) 
{ 
}

wstring const & PartialCopyProgressData::get_Type() const
{
    return Constants::PartialCopyProgressDataType;
}

wstring PartialCopyProgressData::ConstructKey() const
{
    return Constants::PartialCopyProgressDataName;
}

void PartialCopyProgressData::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w << "LastLsn=" << lastStartLsn_;
}
