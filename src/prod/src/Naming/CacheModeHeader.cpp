// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    
    CacheModeHeader::CacheModeHeader ()
    {
    }

    CacheModeHeader::CacheModeHeader(Reliability::CacheMode::Enum cacheMode)
        : cacheMode_(cacheMode)
        , previousVersion_()
    {
    }

    CacheModeHeader::CacheModeHeader(ServiceLocationVersion const & previousVersion)
        : cacheMode_(Reliability::CacheMode::UseCached)
        , previousVersion_(previousVersion)
    {
    }

    CacheModeHeader::CacheModeHeader(Reliability::CacheMode::Enum cacheMode, ServiceLocationVersion const & previousVersion)
        : cacheMode_(cacheMode)
        , previousVersion_(previousVersion)
    {
    }

    void CacheModeHeader::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w << cacheMode_ << L"(version=" << previousVersion_ << L")";
    }

    wstring CacheModeHeader::ToString() const
    {
        wstring result;
        StringWriter writer(result);
        writer.Write("{0}", cacheMode_);
        return result;
    }
}
