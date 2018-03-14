// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace Common;

    namespace ArbitrationType
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
                case TwoWaySimple:
                    w << "TwoWaySimple";
                    return;
                case TwoWayExtended:
                    w << "TwoWayExtended";
                    return;
                case OneWay: 
                    w << "OneWay";
                    return;
                case RevertToReject:
                    w << "RevertToReject";
                    return;
                case RevertToNeutral:
                    w << "RevertToNeutral";
                    return;
                case Implicit:
                    w << "Implicit";
                    return;
                case Query:
                    w << "Query";
                    return;
                case KeepAlive:
                    w << "KeepAlive";
                    return;
                default:
                    w << "InvalidValue(" << static_cast<int>(val) << ')';
                    return;
            }
        }
    }
}

