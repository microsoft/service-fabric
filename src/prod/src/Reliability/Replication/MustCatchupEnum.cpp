// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
namespace Reliability
{
    namespace ReplicationComponent
    {
        using namespace std;
        using namespace Common;

        namespace MustCatchup
        {
            void WriteToTextWriter(TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Unknown:
                    w << "Unknown";
                    break;

                case No:
                    w << "No";
                    break;

                case Yes:
                    w << "Yes";
                    break;

                default:
                    Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
                };
            }

            bool AreEqual(Enum value, bool mustCatchup)
            {
                if ((value == Yes && mustCatchup == true) ||
                    (value == No && mustCatchup == false))
                {
                    return true;
                }
                 
                return false;
            }

            ENUM_STRUCTURED_TRACE(MustCatchup, Unknown, LastValidEnum);
        }
    }
}
