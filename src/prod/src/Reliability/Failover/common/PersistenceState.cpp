// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

void Reliability::PersistenceState::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case NoChange: 
            w << "NoChange"; return;
        case ToBeUpdated: 
            w << "ToBeUpdated"; return;
        case ToBeInserted: 
            w << "ToBeInserted"; return;
        case ToBeDeleted: 
            w << "ToBeDeleted"; return;
        default: 
            Assert::CodingError("Unknown Replica State");
    }
}

