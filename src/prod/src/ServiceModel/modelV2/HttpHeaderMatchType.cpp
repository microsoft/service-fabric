//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace HeaderMatchType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::Invalid: w << "Invalid"; return;
                case Enum::Exact: w << "Exact"; return;
                }
                w << "HeaderMatchType({0}" << static_cast<int>(e) << ')';
            }
        }
    }
}
