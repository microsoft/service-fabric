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
    namespace EntryPointType
    {
        void EntryPointType::WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case EntryPointType::None:
                w << L"None";
                return;
            case EntryPointType::Exe:
                w << L"Exe";
                return;
            case EntryPointType::DllHost:
                w << L"DllHost";
                return;
            case EntryPointType::ContainerHost:
                w << L"ContainerHost";
                return;
            default:
                Assert::CodingError("Unknown EntryPointType value {0}", (int)val);
            }
        }

        ErrorCode EntryPointType::FromPublicApi(FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND const & publicVal, __out Enum & val)
        {
            switch (publicVal)
            {
            case FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_NONE:
                val = EntryPointType::None;
                break;
            case FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST:
                val = EntryPointType::Exe;
                break;
            case FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST:
                val = EntryPointType::DllHost;
                break;
            case FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_CONTAINERHOST:
                val = EntryPointType::ContainerHost;
                break;
            default:
                return ErrorCode::FromHResult(E_INVALIDARG);
            }

            return ErrorCode(ErrorCodeValue::Success);
        }

        FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND EntryPointType::ToPublicApi(Enum const & val)
        {
            switch (val)
            {
            case EntryPointType::None:
                return FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_NONE;
            case EntryPointType::Exe:
                return FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST;
            case EntryPointType::DllHost:
                return FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST;
            case EntryPointType::ContainerHost:
                return FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_CONTAINERHOST;
            default:
                Assert::CodingError("Unknown EntryPointType value {0}", (int)val);
            }
        }

        bool EntryPointType::TryParseFromString(std::wstring const& string, __out Enum & val)
        {
            val = EntryPointType::None;

            if (string == L"Exe")
            {
                val = EntryPointType::Exe;
            }
            else if (string == L"DllHost")
            {
                val = EntryPointType::DllHost;
            }
            else if (string == L"ContainerHost")
            {
                val = EntryPointType::ContainerHost;
            }
            else
            {
                return false;
            }

            return true;
        }

        ENUM_STRUCTURED_TRACE(EntryPointType, None, ContainerHost);
    }
}
