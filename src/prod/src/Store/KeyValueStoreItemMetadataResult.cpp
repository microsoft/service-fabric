// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

KeyValueStoreItemMetadataResult::KeyValueStoreItemMetadataResult(
    wstring const & key,
    LONG valueSizeInBytes,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    FILETIME lastModifiedUtc,
    FILETIME lastModifiedOnPrimaryUtc)
    : ComponentRoot(),
    IKeyValueStoreItemMetadataResult(),
    key_(key),
    itemMetadata_(),
    itemMetadataEx1_()
{
    // initialize item metadata
    itemMetadataEx1_.LastModifiedOnPrimaryUtc = lastModifiedOnPrimaryUtc;
    itemMetadataEx1_.Reserved = NULL;

    itemMetadata_.Reserved = &itemMetadataEx1_;
    itemMetadata_.Key = key_.c_str();
    itemMetadata_.LastModifiedUtc = lastModifiedUtc;
    itemMetadata_.SequenceNumber = sequenceNumber;
    itemMetadata_.ValueSizeInBytes = valueSizeInBytes;
}

KeyValueStoreItemMetadataResult::~KeyValueStoreItemMetadataResult()
{
}

FABRIC_KEY_VALUE_STORE_ITEM_METADATA  const * KeyValueStoreItemMetadataResult::get_Metadata()
{
    return &itemMetadata_;
}
