// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeleteApplicationDescription : public Serialization::FabricSerializable
    {
    public:
        DeleteApplicationDescription();
        explicit DeleteApplicationDescription(
            Common::NamingUri const & applicationName);
        DeleteApplicationDescription(
            Common::NamingUri const & applicationName,
            bool isForce);
        DeleteApplicationDescription(
            DeleteApplicationDescription const & other);
        virtual ~DeleteApplicationDescription() {};

        DeleteApplicationDescription operator=(
            DeleteApplicationDescription const & other);

        Common::ErrorCode FromPublicApi(FABRIC_DELETE_APPLICATION_DESCRIPTION const & appDeleteDesc);

        __declspec(property(get=get_IsForce)) bool IsForce;
        __declspec(property(get=get_ApplicationName, put=put_ApplicationName)) Common::NamingUri const & ApplicationName;

        bool get_IsForce() const { return isForce_; }
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(Common::NamingUri const & value) { applicationName_ = value; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext &) const;
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(applicationName_, isForce_);

    protected:
        Common::NamingUri applicationName_;
        bool isForce_;
    };
}
