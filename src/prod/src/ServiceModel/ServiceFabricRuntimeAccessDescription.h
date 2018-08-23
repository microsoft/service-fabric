//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceFabricRuntimeAccessDescription
    {
    public:
        ServiceFabricRuntimeAccessDescription();
        ServiceFabricRuntimeAccessDescription(ServiceFabricRuntimeAccessDescription const & other);
        ServiceFabricRuntimeAccessDescription(ServiceFabricRuntimeAccessDescription && other);

        ServiceFabricRuntimeAccessDescription const & operator = (ServiceFabricRuntimeAccessDescription const & other);
        ServiceFabricRuntimeAccessDescription const & operator = (ServiceFabricRuntimeAccessDescription && other);

        bool operator == (ServiceFabricRuntimeAccessDescription const & other) const;
        bool operator != (ServiceFabricRuntimeAccessDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ReadFromXml(Common::XmlReaderUPtr const &);

    public:
        bool RemoveServiceFabricRuntimeAccess;
        bool UseServiceFabricReplicatedStore;
    };
}
