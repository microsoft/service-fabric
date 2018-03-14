// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

InternalListReply::InternalListReply()
    : isPresent_()
    , state_()
    , currentVersion_()
    , storeShareLocation_()
{
}

InternalListReply::InternalListReply(
    bool isPresent, 
    FileState::Enum const state, 
    StoreFileVersion const & currentVersion, 
    std::wstring const & storeShareLocation)
    : isPresent_(isPresent)
    , state_(state)
    , currentVersion_(currentVersion)
    , storeShareLocation_(storeShareLocation)
{
}

void InternalListReply::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.WriteLine(
        "InternalListReply{IsPresent={0}, State={1}, CurrentVersion={2}, StoreShareLocation={3}}",
        isPresent_,
        state_,
        currentVersion_,
        storeShareLocation_);
}
