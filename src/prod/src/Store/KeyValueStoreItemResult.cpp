// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

KeyValueStoreItemResult::KeyValueStoreItemResult(
    wstring const & key,
    vector<BYTE> && value,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    FILETIME lastModifiedUtc,
    FILETIME lastModifiedOnPrimaryUtc)
    : ComponentRoot(),
    IKeyValueStoreItemResult(),
    key_(key),
    value_(move(value)),
    itemMetadata_(),
    item_(),
    itemMetadataEx1_()
{
    // initialize item metadata
    itemMetadataEx1_.Reserved = NULL;
    itemMetadataEx1_.LastModifiedOnPrimaryUtc = lastModifiedOnPrimaryUtc;

    itemMetadata_.Reserved = &itemMetadataEx1_;
    itemMetadata_.Key = key_.c_str();
    itemMetadata_.LastModifiedUtc = lastModifiedUtc;
    itemMetadata_.SequenceNumber = sequenceNumber;
    itemMetadata_.ValueSizeInBytes = static_cast<LONG>(value_.size());

    // initialize item
    item_.Reserved = NULL;
    item_.Metadata = &itemMetadata_;
    item_.Value = value_.data();
}

KeyValueStoreItemResult::~KeyValueStoreItemResult()
{
}

FABRIC_KEY_VALUE_STORE_ITEM const * KeyValueStoreItemResult::get_Item()
{
    return &item_;
}
