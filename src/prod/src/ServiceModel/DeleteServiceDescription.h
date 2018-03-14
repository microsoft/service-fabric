// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeleteServiceDescription : public Serialization::FabricSerializable
    {
    public:
        DeleteServiceDescription();
        explicit DeleteServiceDescription(
            Common::NamingUri const & serviceName);
        DeleteServiceDescription(
            Common::NamingUri const & serviceName,
            bool isForce);
        DeleteServiceDescription(
            DeleteServiceDescription const & other);
        virtual ~DeleteServiceDescription() {};

        DeleteServiceDescription operator=(
            DeleteServiceDescription const & other);

        Common::ErrorCode FromPublicApi(FABRIC_DELETE_SERVICE_DESCRIPTION const & svcDeleteDesc);

        __declspec(property(get=get_IsForce)) bool IsForce;
        __declspec(property(get=get_ServiceName, put=put_ServiceName)) Common::NamingUri const & ServiceName;

        bool get_IsForce() const { return isForce_; }
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }
        void put_ServiceName(Common::NamingUri const & value) { serviceName_ = value; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext &) const;
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(serviceName_, isForce_);

    protected:
        Common::NamingUri serviceName_;
        bool isForce_;
    };
}
