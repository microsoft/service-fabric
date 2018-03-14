// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    INITIALIZE_SIZE_ESTIMATION(NamePropertyMetadataResult)

    NamePropertyMetadataResult::NamePropertyMetadataResult()
        : NamePropertyMetadata()
        , lastModifiedUtc_(DateTime::Zero)
        , sequenceNumber_(0)
        , operationIndex_(0)
        , name_()
    {
    }

    NamePropertyMetadataResult::NamePropertyMetadataResult(
        NamePropertyMetadataResult && other)
        : NamePropertyMetadata(move(other))
        , lastModifiedUtc_(move(other.lastModifiedUtc_))
        , sequenceNumber_(move(other.sequenceNumber_))
        , operationIndex_(move(other.operationIndex_))
        , name_(move(other.name_))
    {
    }

    NamePropertyMetadataResult::NamePropertyMetadataResult(
        NamingUri const & name,
        NamePropertyMetadata && metadata,
        DateTime lastModifiedUtc,
        _int64 sequenceNumber,
        size_t operationIndex)
        : NamePropertyMetadata(move(metadata))
        , lastModifiedUtc_(lastModifiedUtc)
        , sequenceNumber_(sequenceNumber)
        , operationIndex_(static_cast<ULONG>(operationIndex))
        , name_(name)
    {
    }

    void NamePropertyMetadataResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_NAMED_PROPERTY_METADATA & metadataResult) const
    {
        metadataResult.PropertyName = heap.AddString(this->PropertyName);
        metadataResult.TypeId = this->TypeId;
        metadataResult.ValueSize = this->SizeInBytes;
        metadataResult.SequenceNumber = this->SequenceNumber;
        metadataResult.LastModifiedUtc = this->LastModifiedUtc.AsFileTime;
        metadataResult.Name = heap.AddString(this->Name.ToString());
         
        // Reserved is set to NULL by default by ScopedHeap
        if (!this->CustomTypeId.empty())
        {
            auto extended = heap.AddItem<FABRIC_NAMED_PROPERTY_METADATA_EX1>().GetRawPointer();
            extended->CustomTypeId = heap.AddString(this->CustomTypeId);
            metadataResult.Reserved = extended;
        }
    }
}
