// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Naming
{ 
    class ServiceGroupServiceDescription
    {
    public:

        ServiceGroupServiceDescription();

        HRESULT FromServiceGroupDescription(__in FABRIC_SERVICE_GROUP_DESCRIPTION const & description);

        FABRIC_SERVICE_DESCRIPTION * GetRawPointer();

    private:

        HRESULT ProcessServiceDescription(__in FABRIC_SERVICE_GROUP_DESCRIPTION const & description);
        HRESULT ProcessMembers(__in FABRIC_SERVICE_GROUP_DESCRIPTION const & description);
        HRESULT ProcessMemberDescription(__in FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION const & description, __in size_t memberIndex);
        HRESULT UpdateInitializationData();
        void UpdateLoadMetrics();

        std::map<std::wstring, FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> loadMetrics_;
        
        HRESULT AddLoadMetric(__in FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION const & loadMetric);

        std::set<Common::NamingUri> serviceInstanceNames_;
        Common::NamingUri nameServiceGroupUri_;

        
        std::vector<byte> serializedServiceGroupDescription_;
        
        Common::ReferencePointer<FABRIC_SERVICE_DESCRIPTION> serviceDescription_;
        ServiceModel::CServiceGroupDescription serviceGroupDescription_;

        Common::ScopedHeap heap_;
    };
}
