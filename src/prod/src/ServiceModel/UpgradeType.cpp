// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace ServiceModel
{
    namespace UpgradeType
    {
        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Invalid:
                w << L"Invalid";
                return;
            case Rolling:
                w << L"Rolling";
                return;
            case Rolling_ForceRestart:
                w << L"Rolling_ForceRestart";
                return;
            case Rolling_NotificationOnly:
                w << L"Rolling_NotificationOnly";
                return;
            default:
                Common::Assert::CodingError("Unknown UpgradeType value {0}", static_cast<uint>(val));
            }
        }

        ENUM_STRUCTURED_TRACE( UpgradeType, Invalid, LastValidEnum )
    }
}
