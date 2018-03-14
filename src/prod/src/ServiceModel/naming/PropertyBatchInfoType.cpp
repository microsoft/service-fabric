// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Naming
{
    namespace PropertyBatchInfoType
    {
        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val)
        {
            switch (val)
            {
                case Invalid: { w << L"Invalid"; return; }
                case Successful: { w << L"Successful"; return; }
                case Failed: { w << L"Failed"; return; }
                default:
                {
                    Common::Assert::CodingError("Unknown PropertyBatchInfoKind value {0}", static_cast<uint>(val));
                }
            }
        }
    }
}
