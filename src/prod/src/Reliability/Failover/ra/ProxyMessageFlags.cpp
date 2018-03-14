// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyMessageFlags
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                if (val == None)
                {
                    w << "-";
                    return;
                }

                if (val & EndReconfiguration)
                {
                    w << "ER|";
                }

                if (val & Catchup)
                {
                    w << "C|";
                }

                if (val & Abort)
                {
                    w << "A|";
                }

                if (val & Drop)
                {
                    w << "D|";
                }
            }

            ENUM_STRUCTURED_TRACE(ProxyMessageFlags, None, LastValidEnum);
        }
    }
}

