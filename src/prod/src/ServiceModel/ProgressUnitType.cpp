// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace ServiceModel
{
    namespace ProgressUnitType
    {
        FABRIC_PROGRESS_UNIT_TYPE ToPublicApi(Enum val)
        {
            switch (val)
            {
            case Bytes:
                return FABRIC_PROGRESS_UNIT_TYPE_BYTES;

            case ServiceSubPackages:
                return FABRIC_PROGRESS_UNIT_TYPE_SERVICE_SUB_PACKAGES;

            case Files:
                return FABRIC_PROGRESS_UNIT_TYPE_FILES;

            default:
                return FABRIC_PROGRESS_UNIT_TYPE_INVALID;
            }
        }

        Enum FromPublicApi(FABRIC_PROGRESS_UNIT_TYPE val)
        {
            switch (val)
            {
            case FABRIC_PROGRESS_UNIT_TYPE_BYTES:
                return Bytes;

            case FABRIC_PROGRESS_UNIT_TYPE_SERVICE_SUB_PACKAGES:
                return ServiceSubPackages;

            case FABRIC_PROGRESS_UNIT_TYPE_FILES:
                return Files;

            default:
                return Invalid;
            }
        }

        void WriteToTextWriter(__in Common::TextWriter & w, Enum val)
        {
            switch (val)
            {
            case Invalid:
                w << L"Invalid";
                return;

            case Bytes:
                w << L"Bytes";
                return;

            case Files:
                w << L"Files";
                return;

            default:
                w << L"ProgressUnitType(" << static_cast<uint>(val) << L")";
            }
        }

        ENUM_STRUCTURED_TRACE( ProgressUnitType, Invalid, LastValidEnum )
    }
}

