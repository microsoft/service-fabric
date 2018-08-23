//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace FabricTypeHost
{

    class EndpointsDescription
        : public Common::IFabricJsonSerializable
    {
    public:
        EndpointsDescription();

        ~EndpointsDescription();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY_SIMPLE_MAP(EndpointsDescription::EndpointsParameter, Endpoints)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::map<std::wstring, std::wstring> Endpoints;
        static Common::WStringLiteral const EndpointsParameter;
    };
}
