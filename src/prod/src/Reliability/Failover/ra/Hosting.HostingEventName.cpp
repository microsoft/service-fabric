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
        namespace Hosting
        {
            namespace HostingEventName
            {
                void WriteToTextWriter(TextWriter & w, HostingEventName::Enum const & e)
                {
                    switch (e)
                    {
                    case ServiceTypeRegistered:
                        w << "ServiceTypeRegistered";
                        break;

                    case AppHostDown:
                        w << "AppHostDown";
                        break;

                    case RuntimeDown:
                        w << "RuntimeDown";
                        break;

                    default:
                        Assert::CodingError("Unknown value {0}", static_cast<int>(e));
                    }
                }

                ENUM_STRUCTURED_TRACE(HostingEventName, ServiceTypeRegistered, LastValidEnum);
            }
        }
    }
}
