//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Query
{
    class NamesArgument
        : public Common::IFabricJsonSerializable
    {
        DENY_COPY(NamesArgument)

    public:
        NamesArgument()
            :Names()
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Names, Names)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::vector<std::wstring> Names;
    };
}
