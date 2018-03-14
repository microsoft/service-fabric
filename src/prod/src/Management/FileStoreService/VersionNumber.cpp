// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

VersionNumber::VersionNumber()
    : versionNumber_(0)
{
}
 VersionNumber::VersionNumber(uint64 const versionNumber)
     : versionNumber_(versionNumber)
 {
 }

 VersionNumber::~VersionNumber()
 {
 }

 wstring VersionNumber::ConstructKey() const
{
    return Constants::StoreType::LastDeletedVersionNumber;
}

void VersionNumber::WriteTo(TextWriter & w, FormatOptions const &) const
{    
    w.Write("VersionNumber[{0}]", versionNumber_);
}
