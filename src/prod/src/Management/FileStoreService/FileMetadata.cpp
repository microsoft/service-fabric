// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

FileMetadata::FileMetadata()
    : storeRelativeLocation_()
    , currentVersion_()
    , previousVersion_()
    , state_()
    , copyDesc_()
{
}

FileMetadata::FileMetadata(std::wstring const & storeRelativeLocation)
    : storeRelativeLocation_(storeRelativeLocation)
    , currentVersion_()
    , previousVersion_()
    , state_()
    , copyDesc_()
{
}

FileMetadata::FileMetadata(std::wstring const & storeRelativeLocation, StoreFileVersion const currentVersion, FileState::Enum const state)
: storeRelativeLocation_(storeRelativeLocation)
, currentVersion_(currentVersion)
, previousVersion_()
, state_(state)
, copyDesc_()
{
}

FileMetadata::FileMetadata(std::wstring const & storeRelativeLocation, StoreFileVersion const currentVersion, FileState::Enum const state, CopyDescription const & copyDesc)
    : storeRelativeLocation_(storeRelativeLocation)
    , currentVersion_(currentVersion)
    , previousVersion_()
    , state_(state)
    , copyDesc_(copyDesc)
{
}

FileMetadata::~FileMetadata()
{
}

wstring FileMetadata::ConstructKey() const
{
#if !defined(PLATFORM_UNIX)
    wstring key = storeRelativeLocation_;
    StringUtility::ToLower(key);
    return key;
#else
    return storeRelativeLocation_;
#endif
}

void FileMetadata::WriteTo(TextWriter & w, FormatOptions const &) const
{    
    w.Write("FileMetadata[{0}, {1}, {2}, {3}, {4}]", storeRelativeLocation_, currentVersion_, previousVersion_, state_, copyDesc_);
}
