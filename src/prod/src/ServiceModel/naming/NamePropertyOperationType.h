// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace NamePropertyOperationType
    {
        enum Enum
        {
            CheckExistence = 0,
            CheckSequence = 1,
            GetProperty = 2,
            PutProperty = 3,
            DeleteProperty = 4,
            EnumerateProperties = 5,
            PutCustomProperty = 6,
            CheckValue = 7,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}

