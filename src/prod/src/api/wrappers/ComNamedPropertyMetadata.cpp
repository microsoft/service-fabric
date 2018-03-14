// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Naming;

ComNamedPropertyMetadata::ComNamedPropertyMetadata(NamePropertyMetadataResult const & metadata)
    : heap_()
    , buffer_()
{
    buffer_ = heap_.AddItem<FABRIC_NAMED_PROPERTY_METADATA>();
    metadata.ToPublicApi(heap_, *buffer_);
}

const FABRIC_NAMED_PROPERTY_METADATA * STDMETHODCALLTYPE ComNamedPropertyMetadata::get_Metadata(void)
{
    return buffer_.GetRawPointer();
}
