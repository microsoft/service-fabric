// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace std;

ComStringMapResult::ComStringMapResult(map<wstring, wstring> && values)
    : map_(move(values))
    , heap_()
    , result_()
    , lock_()
{
}

ComStringMapResult::~ComStringMapResult()
{
}

FABRIC_STRING_PAIR_LIST * STDMETHODCALLTYPE ComStringMapResult::get_Result(void)
{
    {
        AcquireReadLock lock(lock_);

        if (result_.GetRawPointer() != NULL)
        {
            return result_.GetRawPointer();
        }
    }

    AcquireWriteLock lock(lock_);

    if (result_.GetRawPointer() == NULL)
    {
        auto index = 0;
        auto array = heap_.AddArray<FABRIC_STRING_PAIR>(map_.size());
        for (auto const & item : map_)
        {
            array[index].Name = item.first.c_str();
            array[index].Value = item.second.c_str();

            ++index;
        }

        result_ = heap_.AddItem<FABRIC_STRING_PAIR_LIST>();
        result_->Count = static_cast<ULONG>(array.GetCount());
        result_->Items = array.GetRawArray();
    }

    return result_.GetRawPointer();
}
