// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace UpgradeDomainSortPolicy
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const& value)
            {
                switch (value)
                {
                case Lexicographical:
                    w << "Lexicographical";
                    return;
                case DigitsAsNumbers:
                    w << "DigitsAsNumbers";
                    return;
                    return;
                default:
                    w << "Unknown UpgradeDomainSortPolicy: {0}" << static_cast<int>(value);
                }
            }

            ENUM_STRUCTURED_TRACE(UpgradeDomainSortPolicy, Lexicographical, LastValidEnum);
        }
    }
}
