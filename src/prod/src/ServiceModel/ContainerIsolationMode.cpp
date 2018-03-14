// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace ServiceModel
{
    namespace ContainerIsolationMode
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case ContainerIsolationMode::process:
                w << L"process";
                return;
            case ContainerIsolationMode::hyperv:
                w << L"hyperv";
                return;

            default:
                Assert::CodingError("Unknown ContainerIsolationMode value {0}", (int)val);
            }
        }

        ErrorCode FromString(wstring const & value, __out Enum & val)
        {
            if (value == L"process" || value == L"default")
            {
                val = ContainerIsolationMode::process;
                return ErrorCode(ErrorCodeValue::Success);
            }
            else if (value == L"hyperv")
            {
                val = ContainerIsolationMode::hyperv;
                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                return ErrorCode::FromHResult(E_INVALIDARG);
            }
        }

        wstring EnumToString(Enum val)
        {
            switch (val)
            {
            case ContainerIsolationMode::process:
                return L"process";
            case ContainerIsolationMode::hyperv:
                return L"hyperv";
            default:
                Assert::CodingError("Unknown ContainerIsolationMode value {0}", (int)val);
            }
        }

        FABRIC_CONTAINER_ISOLATION_MODE ContainerIsolationMode::ToPublicApi(Enum isolationMode)
        {
            switch (isolationMode)
            {
            case process:
                return FABRIC_CONTAINER_ISOLATION_MODE_PROCESS;

            case hyperv:
                return FABRIC_CONTAINER_ISOLATION_MODE_HYPER_V;

            default:
                return FABRIC_CONTAINER_ISOLATION_MODE_PROCESS;
            }
        }
    }
}
