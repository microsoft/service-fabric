// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class CancelTestCommandDescription
            : public Serialization::FabricSerializable
        {
        public:

            CancelTestCommandDescription();
            CancelTestCommandDescription(Common::Guid &operationId, bool force);
            CancelTestCommandDescription(CancelTestCommandDescription const &);
            CancelTestCommandDescription(CancelTestCommandDescription &&);

            __declspec(property(get=get_OperationId)) Common::Guid const & Id;
            __declspec(property(get=get_Force)) bool const & Force;

            Common::Guid const & get_OperationId() const { return operationId_; }
            bool const & get_Force() const { return force_; }

            bool operator == (CancelTestCommandDescription const & other) const;
            bool operator != (CancelTestCommandDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION const &); 

            FABRIC_FIELDS_02(
                operationId_, 
                force_);

        private:

            Common::Guid operationId_;
            bool force_;
        };
    }
}
