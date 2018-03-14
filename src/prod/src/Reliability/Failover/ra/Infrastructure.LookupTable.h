// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            /*
                Stores a lookup table over T

                T must have values in the range [0, Size)

                LookupTable can be built using the LookupTableBuilder
            */
            template<typename T, size_t Size>
            class LookupTable
            {
            public:
                LookupTable(LookupTableValue::Enum values[Size][Size])
                {
                    for (int i = 0; i < Size; i++)
                    {
                        for (int j = 0; j < Size; j++)
                        {
                            values_[i][j] = values[i][j];
                        }
                    }
                }

                bool Lookup(T left, T right) const
                {
                    switch (values_[left][right])
                    {
                    case LookupTableValue::True:
                        return true;
                    case LookupTableValue::False:
                        return false;
                    case LookupTableValue::Invalid:
                        Common::Assert::CodingError("Disallowed lookup table value {0} {1}", static_cast<int>(left), static_cast<int>(right));
                    default:
                        Common::Assert::CodingError("Unknown enum value");
                    }
                }

            private:
                LookupTableValue::Enum values_[Size][Size];
            };
        }
    }
}




