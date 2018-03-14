// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CheckSequencePropertyBatchOperationDescription : public PropertyBatchOperationDescription
    {
        DENY_COPY(CheckSequencePropertyBatchOperationDescription);

    public:
        CheckSequencePropertyBatchOperationDescription()
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE)
            , sequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
        {
        }

        CheckSequencePropertyBatchOperationDescription(
            std::wstring const & propertyName,
            int64 sequenceNumber)
            : PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE, propertyName)
            , sequenceNumber_(sequenceNumber)
        {
        }

        virtual ~CheckSequencePropertyBatchOperationDescription() {};

        Common::ErrorCode Verify()
        {
            if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE)
            {
                return Common::ErrorCode::Success();
            }
            return Common::ErrorCodeValue::InvalidArgument;
        }
        
        __declspec(property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SequenceNumber, sequenceNumber_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        int64 sequenceNumber_;
    };

    using CheckSequencePropertyBatchOperationDescriptionSPtr = std::shared_ptr<CheckSequencePropertyBatchOperationDescription>;

    template<> class PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE>
    {
    public:
        static PropertyBatchOperationDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<CheckSequencePropertyBatchOperationDescription>();
        }
    };
}
