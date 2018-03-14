// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceGroupMemberQueryResult :
        public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
	{
		DENY_COPY(ServiceGroupMemberQueryResult);

	public:
		ServiceGroupMemberQueryResult();
		ServiceGroupMemberQueryResult(ServiceGroupMemberQueryResult && other);
        ServiceGroupMemberQueryResult const & operator = (ServiceGroupMemberQueryResult && other);

        static ServiceGroupMemberQueryResult CreateServiceGroupMemberQueryResult(
            Common::Uri const & serviceName,
            std::vector<CServiceGroupMemberDescription> & cServiceGroupMemberDescriptions);

        __declspec(property(get = get_ServiceGroupMemberDescriptions)) std::vector<CServiceGroupMemberDescription> const &ServiceGroupMemberDescriptions;
        std::vector<CServiceGroupMemberDescription> const& get_ServiceGroupMemberDescriptions() const { return serviceGroupMemberDescriptions_; }

        __declspec(property(get = get_ServiceName)) Common::Uri const &ServiceName;
        Common::Uri const& get_ServiceName() const { return serviceName_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_ITEM & publicServiceGroupMemberQueryResult) const;

        FABRIC_FIELDS_02(serviceName_, serviceGroupMemberDescriptions_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceGroupMemberDescription, serviceGroupMemberDescriptions_)
        END_JSON_SERIALIZABLE_PROPERTIES()

		virtual ~ServiceGroupMemberQueryResult();

	private:
		ServiceGroupMemberQueryResult(
			Common::Uri const & serviceName,
			std::vector<CServiceGroupMemberDescription> & serviceGroupMemberDescriptions);

		std::vector<CServiceGroupMemberDescription> serviceGroupMemberDescriptions_;

        Common::Uri serviceName_;
        FABRIC_QUERY_SERVICE_STATUS serviceStatus_;
	};
}
