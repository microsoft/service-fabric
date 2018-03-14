// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace LogLevel
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Silent: w <<   "Silent  "; return;
                case Critical: w << "Critical"; return;
                case Error: w <<    "Error   "; return;
                case Warning: w <<  "Warning "; return;
                case Info: w <<     "Info    "; return;
                case Noise: w <<    "Noise   "; return;
                case All: w <<      "All     "; return;
            }

            w << "LogLevel(" << (int)e << ')';
        }
    }
}
