// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    
    internal enum PropertyFlags
    {
        PropertyStruct = 0x1,
        PropertyParamLength = 0x2,
        PropertyParamCount = 0x4,
        PropertyWBEMXmlFragment = 0x8,
        PropertyParamFixedLength = 0x10
    }
}