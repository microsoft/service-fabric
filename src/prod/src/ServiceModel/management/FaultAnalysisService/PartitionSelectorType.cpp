// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace PartitionSelectorType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Singleton:
                    w << L"Singleton";
                    return;
                case Named:
                    w << L"Named";
                    return;
                case UniformInt64:
                    w << L"UniformInt64";
                    return;
                case PartitionId:
                    w << L"PartitionId";
                    return;
                case Random:
                    w << L"Random";
                    return;
                default:
                    w << Common::wformatString("Unknown PartitionSelectorType value {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(PartitionSelectorType, None, static_cast<Enum>(LAST_ENUM_PLUS_ONE - 1));

            Enum FromPublicApi(FABRIC_PARTITION_SELECTOR_TYPE const publicMode)
            {
                switch (publicMode)
                {
                case FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON:
                    return Singleton;
                case FABRIC_PARTITION_SELECTOR_TYPE_NAMED:
                    return Named;
                case FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64:
                    return UniformInt64;
                case FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID:
                    return PartitionId;
                case FABRIC_PARTITION_SELECTOR_TYPE_RANDOM:
                    return Random;
                default:
                    return None;
                }
            }

            FABRIC_PARTITION_SELECTOR_TYPE ToPublicApi(Enum const nativeMode)
            {
                switch (nativeMode)
                {
                case Singleton:
                    return FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON;
                case Named:
                    return FABRIC_PARTITION_SELECTOR_TYPE_NAMED;
                case UniformInt64:
                    return FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64;
                case PartitionId:
                    return FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID;
                case Random:
                    return FABRIC_PARTITION_SELECTOR_TYPE_RANDOM;
                default:
                    return FABRIC_PARTITION_SELECTOR_TYPE_NONE;
                }
            }
        }
    }
}
