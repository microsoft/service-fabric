// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Transport
{
    TransportTable::TransportTable(void)
    {
    }

    TransportTable::~TransportTable(void)
    {
    }

    void TransportTable::Add(wstring const & name, weak_ptr<IDatagramTransport> const & item)
    {
        AcquireExclusiveLock lock(this->transportTableLock_);

        TransportLookupTable::iterator iter = this->transportTable_.find(name);

        ASSERT_IF(iter != this->transportTable_.end(), "key already exists");
        
        this->transportTable_[name] = item;
    }

    bool TransportTable::Contains(wstring const & name)
    {
        AcquireExclusiveLock lock(this->transportTableLock_);

        TransportLookupTable::iterator iter = this->transportTable_.find(name);

        return iter != this->transportTable_.end();
    }

    bool TransportTable::Remove(wstring const & name)
    {
        AcquireExclusiveLock lock(this->transportTableLock_);

        TransportLookupTable::iterator iter = this->transportTable_.find(name);
        if (iter != this->transportTable_.end())
        {
            this->transportTable_.erase(iter);

            return true;
        }

        return false;
    }

    bool TransportTable::TryGet(wstring const & name, weak_ptr<IDatagramTransport> & item)
    {
        AcquireExclusiveLock lock(this->transportTableLock_);

        TransportLookupTable::iterator iter = this->transportTable_.find(name);
        if (iter != this->transportTable_.end())
        {
            item = iter->second;

            return true;
        }

        return false;
    }
}
