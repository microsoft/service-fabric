// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedEndpoint> element.
    struct DigestedResourcesDescription;
    struct DigestedEndpointDescription
    {
        DigestedEndpointDescription();
        DigestedEndpointDescription(DigestedEndpointDescription const & other);
        DigestedEndpointDescription(DigestedEndpointDescription && other);

        DigestedEndpointDescription const & operator = (DigestedEndpointDescription const & other);
        DigestedEndpointDescription const & operator = (DigestedEndpointDescription && other);

        bool operator == (DigestedEndpointDescription const & other) const;
        bool operator != (DigestedEndpointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        EndpointDescription Endpoint;
        SecurityAccessPolicyDescription SecurityAccessPolicy;
        EndpointBindingPolicyDescription EndpointBindingPolicy;

    private:
        friend struct DigestedResourcesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
