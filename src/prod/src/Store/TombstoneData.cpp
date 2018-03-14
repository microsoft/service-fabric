// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

TombstoneData::TombstoneData()
    : type_()
    , key_()
    , index_(0)
{ 
}

TombstoneData::TombstoneData(
    std::wstring const & type, 
    std::wstring const & key,
    FABRIC_SEQUENCE_NUMBER lsn,
    size_t index) 
    : type_(type)
    , key_(key)
    , index_(index)
{ 
    this->SetSequenceNumber(lsn);
}

wstring const & TombstoneData::get_Type() const
{
    return Constants::TombstoneDataType;
}

wstring TombstoneData::ConstructKey() const
{
    // Left zero pad so that primary key enumeration order
    // is the same as LSN enumeration order.
    //
    // The order of tombstone cleanup within the same LSN is irrelevant
    // since the low watermark will be set once any cleanup occurs at
    // a given LSN. The index is only used to create a unique key
    // when multiple delete operations occur within the same transaction
    // and therefore, have the same LSN.
    //
    return wformatString("0x{0:016x}:{1}", this->SequenceNumber, index_);
}

wstring const & TombstoneData::get_LiveEntryType() const
{
    return type_;
}

wstring const & TombstoneData::get_LiveEntryKey() const
{
    return key_;
}

size_t TombstoneData::get_Index() const
{
    return index_;
}

bool TombstoneData::operator==(TombstoneData const & other) const
{
    if (&other == this) { return true; }

    if (type_ != other.type_) { return false; }
    if (key_ != other.key_) { return false; }
    if (index_ != other.index_) { return false; }

    return (this->SequenceNumber == other.SequenceNumber);
}

bool TombstoneData::operator!=(TombstoneData const & other) const
{
    return !(*this == other);
}

void TombstoneData::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(
        "tombstone('{0}', '{1}', lsn={2} index={3})", 
        type_, 
        key_, 
        this->SequenceNumber, 
        index_); 
}
