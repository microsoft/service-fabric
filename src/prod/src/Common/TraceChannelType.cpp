// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace TraceChannelType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Admin: w <<       "Admin"; return;
                case Operational: w << "Operational"; return;
                case DeprecatedOperational: w <<    "DeprecatedOperational"; return;
                case Analytic: w <<    "Analytic"; return;
                case Debug: w <<       "Debug"; return;
                default:
                    throw std::runtime_error("Invalid channel type");
            }
        }
    }
}
