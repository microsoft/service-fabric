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
        namespace RetryableErrorStateName
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum e)
            {
                switch (e)
                {
                case None:
                    w << "N";
                    break;

                case ReplicaOpen:
                    w << "O";
                    break;

                case ReplicaReopen:
                    w << "RO";
                    break;

                case FindServiceRegistrationAtOpen:
                    w << "FSO";
                    break;

                case FindServiceRegistrationAtReopen:
                    w << "FSRO";
                    break;

                case FindServiceRegistrationAtDrop:
                    w << "FSRD";
                    break;

                case ReplicaChangeRoleAtCatchup:
                    w << "RC";
                    break;

                case ReplicaClose:
                    w << "C";
                    break;

                case ReplicaDelete:
                    w << "D";
                    break;

                default: 
                    w << "Unknown: " << static_cast<int>(e);
                    break;
                };
            }

            ENUM_STRUCTURED_TRACE( RetryableErrorStateName , None, LastValidEnum );
        }
    }
}
