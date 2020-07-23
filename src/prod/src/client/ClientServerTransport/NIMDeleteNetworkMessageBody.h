// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class DeleteNetworkMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteNetworkMessageBody()
            {
            }

            DeleteNetworkMessageBody(
                ServiceModel::DeleteNetworkDescription const & deleteNetworkDescription)
                : deleteNetworkDescription_(deleteNetworkDescription)
            {
            }

            __declspec(property(get = get_DeleteNetworkDescription)) ServiceModel::DeleteNetworkDescription const & DeleteNetworkDescription;

            ServiceModel::DeleteNetworkDescription const & get_DeleteNetworkDescription() const { return deleteNetworkDescription_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
            {
                w.WriteLine("DeleteNetworkMessageBody: Network: [{0}]", 
                    deleteNetworkDescription_.NetworkName);
            }

            FABRIC_FIELDS_01(deleteNetworkDescription_);

        private:
            ServiceModel::DeleteNetworkDescription deleteNetworkDescription_;
        };
    }
}