// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace Common;

    namespace NodePhase
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
                case Booting: 
                    w << "B";
                    return;
                case Joining: 
                    w << "J";
                    return;
                case Inserting: 
                    w << "I";
                    return;
                case Routing: 
                    w << "R";
                    return;
                case Shutdown: 
                    w << "S";
                    return;
                default: 
                    w << "InvalidValue(" << static_cast<int>(val) << ')';
                    return;
            }
        }
    }
}

