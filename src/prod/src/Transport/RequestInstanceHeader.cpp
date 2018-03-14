// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Transport
{
    RequestInstanceHeader::RequestInstanceHeader() 
        : instance_(-1)
    { 
    }

    RequestInstanceHeader::RequestInstanceHeader(__int64 instance)
        : instance_(instance)
    { 
    }

    RequestInstanceHeader::RequestInstanceHeader(RequestInstanceHeader const & other) 
        : instance_(other.instance_)
    { 
    }
    
    void RequestInstanceHeader::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
    {
        w << "RequestInstance(" << instance_ << ")";
    }

    wstring RequestInstanceHeader::ToString() const
    {
        wstring result;
        StringWriter(result).Write(*this);
        return result;
    }
}
