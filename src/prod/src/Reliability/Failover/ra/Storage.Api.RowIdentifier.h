// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            namespace Api
            {
                class RowIdentifier
                {
                public:
                    RowIdentifier() :
                        rowType_(RowType::Invalid)
                    {
                    }

                    RowIdentifier(RowType::Enum rowType, std::wstring const & id) :
                        rowType_(rowType),
                        id_(id)
                    {
                    }

                    RowIdentifier(RowType::Enum rowType, std::wstring && id) :
                        rowType_(rowType),
                        id_(std::move(id))
                    {
                    }

                    __declspec(property(get = get_Id)) std::wstring const & Id;
                    std::wstring const & get_Id() const { return id_; }

                    __declspec(property(get = get_Type)) RowType::Enum Type;
                    RowType::Enum get_Type() const { return rowType_; }

                    __declspec(property(get = get_TypeString)) std::wstring TypeString;
                    std::wstring get_TypeString() const { return Common::wformatString(rowType_); }     

                    bool operator<(RowIdentifier const & right)
                    {
                        return id_ < right.id_;
                    }

                    void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
                    {
                        w << id_;
                    }

                private:
                    RowType::Enum rowType_;
                    std::wstring id_;
                };

                inline bool operator<(RowIdentifier const & left, RowIdentifier const & right)
                {
                    return left.Id < right.Id;
                }
            }
        }
    }
}


