// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyBatchDescription : public Common::IFabricJsonSerializable
    {
    public:
        PropertyBatchDescription()
            : operations_()
        {
        }

        explicit PropertyBatchDescription(std::vector<PropertyBatchOperationDescriptionSPtr> && operations)
            : operations_(std::move(operations))
        {
        }

        virtual ~PropertyBatchDescription() {};

        Common::ErrorCode Verify()
        {
            for (auto operation : operations_)
            {
                auto error = operation->Verify();
                if (!error.IsSuccess())
                {
                    return error;
                }
            }
            return Common::ErrorCode::Success();
        }
        
        __declspec(property(get=get_Operations)) std::vector<PropertyBatchOperationDescriptionSPtr> const & Operations;
        std::vector<PropertyBatchOperationDescriptionSPtr> const & get_Operations() const { return operations_; }

        std::vector<PropertyBatchOperationDescriptionSPtr> && TakeOperations() { return std::move(operations_); }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Operations, operations_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<PropertyBatchOperationDescriptionSPtr> operations_;
    };
}
