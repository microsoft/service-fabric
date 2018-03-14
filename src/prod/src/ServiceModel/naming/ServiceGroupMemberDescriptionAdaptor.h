// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceGroupMemberDescriptionAdaptor : public Common::IFabricJsonSerializable
    {
    public:

        std::wstring const& getServiceName () {return serviceName_;}
        std::wstring const& getServiceTypeName () {return serviceTypeName_;}
        Common::ByteBuffer const& getInitializationData () {return initializationData_;}
        std::vector<Reliability::ServiceLoadMetricDescription> getMetrics () {return metrics_;}
        void setServiceName (std::wstring serviceName) {serviceName_ = serviceName;}
        void setServiceTypeName (std::wstring serviceTypeName) {serviceTypeName_ = serviceTypeName;}
        void setInitializationData (Common::ByteBuffer initializationData) {initializationData_ = initializationData;}
        void setMetrics (std::vector<Reliability::ServiceLoadMetricDescription> const& metrics) {metrics_ = metrics;}

        ServiceGroupMemberDescriptionAdaptor () {}
        ServiceGroupMemberDescriptionAdaptor(const ServiceGroupMemberDescriptionAdaptor &obj);
        ServiceGroupMemberDescriptionAdaptor& operator=(const ServiceGroupMemberDescriptionAdaptor& obj);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, initializationData_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceLoadMetrics, metrics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceName_;
        std::wstring serviceTypeName_;
        Common::ByteBuffer initializationData_;
        std::vector<Reliability::ServiceLoadMetricDescription> metrics_;
    };
}
