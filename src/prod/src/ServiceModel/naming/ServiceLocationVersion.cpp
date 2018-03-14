// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;

    INITIALIZE_SIZE_ESTIMATION(ServiceLocationVersion)

    int64 ServiceLocationVersion::CompareTo(ServiceLocationVersion const & other) const
    {
        if (generationNumber_ < other.generationNumber_)
        {
            return -1;
        }
        else if (generationNumber_ == other.generationNumber_)
        {
            if (fmVersion_ < other.fmVersion_)
            {
                return -1;
            }
            else if (fmVersion_ == other.fmVersion_)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
        else
        {
            return 1;
        }
    }

    void ServiceLocationVersion::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "ServiceLocationVersion(generation=" << generationNumber_
            << ",fmVer=" << fmVersion_
            << ",psdVersion=" << storeVersion_
            << ")";
    }

    std::string ServiceLocationVersion::AddField(Common::TraceEvent & traceEvent, std::string const & name)
    {
        traceEvent.AddField<int64>(name + ".generation");
        traceEvent.AddField<__int64>(name + ".fmVer");
        traceEvent.AddField<__int64>(name + ".psdVer");
            
        return "ServiceLocationVersion(generation={0}, fmVer={1}, psdVer={2})";
    }

    void ServiceLocationVersion::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<int64>(generationNumber_.Value);
        context.Write<__int64>(fmVersion_);
        context.Write<__int64>(storeVersion_);
    }

    std::wstring ServiceLocationVersion::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
