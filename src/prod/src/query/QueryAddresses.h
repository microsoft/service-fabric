// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryAddresses
    {
    public:

        static Common::GlobalWString FMAddressSegment;
        static Common::GlobalWString FMMAddressSegment;
        static Common::GlobalWString CMAddressSegment;
        static Common::GlobalWString HMAddressSegment;
        static Common::GlobalWString HostingAddressSegment;
        static Common::GlobalWString RAAddressSegment;
        static Common::GlobalWString RMAddressSegment;
        static Common::GlobalWString TSAddressSegment;
        static Common::GlobalWString UOSAddressSegment;
        static Common::GlobalWString TestabilityAddressSegment;
        static Common::GlobalWString GRMAddressSegment;

        static Query::QueryAddressGenerator GetGateway();
        static Query::QueryAddressGenerator GetHosting();
        static Query::QueryAddressGenerator GetFM();
        static Query::QueryAddressGenerator GetFMM();
        static Query::QueryAddressGenerator GetCM(); 
        static Query::QueryAddressGenerator GetHM(); 
        static Query::QueryAddressGenerator GetHMViaCM();
        static Query::QueryAddressGenerator GetRA(); 
        static Query::QueryAddressGenerator GetRM();
        static Query::QueryAddressGenerator GetTS();
        static Query::QueryAddressGenerator GetUOS();
        static Query::QueryAddressGenerator GetTestability();
        static Query::QueryAddressGenerator GetGRM();
    };
}
