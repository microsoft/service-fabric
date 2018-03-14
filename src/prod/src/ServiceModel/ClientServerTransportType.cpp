// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace ServiceModel
{
    namespace ClientServerTransportType
    {
        bool IsValid(Enum value)
        {
            return (value == TcpTransport) || (value == HttpTransport) || (value == PassThroughTransport);
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case UnknownTransport: w << L"UnknownTransport"; return;
                case TcpTransport: w << L"TcpTransport"; return;
                case HttpTransport: w << L"HttpTransport"; return;
                case PassThroughTransport: w << L"PassThroughTransport"; return;
            }

            w << "InvalidTransportType(" << static_cast<int>(e) << ')';
        }
    }
}
