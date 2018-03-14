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
            template<typename T, size_t Size>
            class LookupTableBuilder
            {
            public:
                LookupTableBuilder(bool defaultValue)
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        for (size_t j = 0; j < Size; j++)
                        {
                            values_[i][j] = defaultValue ? LookupTableValue::True : LookupTableValue::False;
                        }
                    }
                }

                void MarkInvalid(std::initializer_list<T> invalidValues)
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        for (size_t j = 0; j < Size; j++)
                        {
                            for (auto const & it : invalidValues)
                            {
                                if (i == it || j == it)
                                {
                                    values_[i][j] = LookupTableValue::Invalid;
                                    break;
                                }
                            }
                        }
                    }
                }

                void MarkTrue(T left, T right)
                {
                    values_[left][right] = LookupTableValue::True;
                }

                void MarkFalse(T left, T right)
                {
                    values_[left][right] = LookupTableValue::False;
                }
    
                LookupTable<T, Size> CreateTable()
                {
                    return LookupTable<T, Size>(values_);
                }

            private:
                LookupTableValue::Enum values_[Size][Size];
            };
        }
    }
}



