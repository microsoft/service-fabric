// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class DeleteNameHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::DeleteName>
        , public Serialization::FabricSerializable
    {
    public:
        DeleteNameHeader() : isExplicit_(true)
        {
        }

        explicit DeleteNameHeader(bool isExplicit) : isExplicit_(isExplicit)
        { 
        }

        __declspec (property(get=get_IsExplicit)) bool IsExplicit;
        bool get_IsExplicit() { return isExplicit_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << "DeleteNameHeader( explicit = " << isExplicit_ << " )";
        }

        FABRIC_FIELDS_01(isExplicit_);

    private:
        bool isExplicit_;
    };
}
