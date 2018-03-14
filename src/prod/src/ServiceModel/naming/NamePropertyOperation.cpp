// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;

    StringLiteral const PropertyOperationTraceType("PropertyOperation");

    NamePropertyOperation::NamePropertyOperation()
        : operationType_()
        , propertyName_()
        , propertyTypeId_()
        , boolParam_(false)
        , bytesParam_()
    {
    }
    
    NamePropertyOperation::NamePropertyOperation(
        NamePropertyOperationType::Enum operationType,
        std::wstring const & propertyName)
        : operationType_(operationType)
        , propertyName_(propertyName)
        , propertyTypeId_(::FABRIC_PROPERTY_TYPE_INVALID)
        , boolParam_(false)
        , int64Param_(0)
        , bytesParam_()
        , customTypeId_()
    {
    }

     NamePropertyOperation::NamePropertyOperation(
        NamePropertyOperationType::Enum operationType,
        std::wstring const & propertyName,
        ::FABRIC_PROPERTY_TYPE_ID propertyTypeId,
        std::vector<byte> && value)
        : operationType_(operationType)
        , propertyName_(propertyName)
        , propertyTypeId_(propertyTypeId)
        , boolParam_(false)
        , int64Param_(0)
        , bytesParam_(move(value))
        , customTypeId_()
    {
    }

    NamePropertyOperation::NamePropertyOperation(
        NamePropertyOperationType::Enum operationType,
        std::wstring const & propertyName,
        ::FABRIC_PROPERTY_TYPE_ID propertyTypeId,
        std::vector<byte> && value,
        std::wstring && customTypeId)
        : operationType_(operationType)
        , propertyName_(propertyName)
        , propertyTypeId_(propertyTypeId)
        , boolParam_(false)
        , int64Param_(0)
        , bytesParam_(move(value))
        , customTypeId_(move(customTypeId))
    {
    }
              
    void NamePropertyOperation::WriteToEtw(uint16 contextSequenceId) const
    {
        switch(operationType_)
        {
        case NamePropertyOperationType::PutProperty:
            ServiceModel::ServiceModelEventSource::Trace->NamePropPutOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_,
                Common::wformatString(static_cast<int>(propertyTypeId_)),
                customTypeId_);
            break;

        case NamePropertyOperationType::GetProperty:
            ServiceModel::ServiceModelEventSource::Trace->NamePropGetOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_,
                boolParam_);
            break;

        case NamePropertyOperationType::DeleteProperty:
            ServiceModel::ServiceModelEventSource::Trace->NamePropOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_);
            break;

        case NamePropertyOperationType::CheckExistence:
            ServiceModel::ServiceModelEventSource::Trace->NamePropCheckExistsOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_,
                boolParam_);
            break;

        case NamePropertyOperationType::CheckSequence:
            ServiceModel::ServiceModelEventSource::Trace->NamePropCheckSequenceOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_,
                int64Param_);
            break;
                
        case NamePropertyOperationType::CheckValue:
            ServiceModel::ServiceModelEventSource::Trace->NamePropCheckValueOp(
                contextSequenceId,
                Common::wformatString(operationType_),
                propertyName_,
                Common::wformatString(static_cast<int>(propertyTypeId_)));
            break;

        default:
            ServiceModel::ServiceModelEventSource::Trace->NamePropOpDefault(
                contextSequenceId,
                Common::wformatString(operationType_));
            break;
        }
    }

    void NamePropertyOperation::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        switch(operationType_)
        {
        case NamePropertyOperationType::PutProperty: 
            w.Write("({0}:{1}:{2}:'{3}')", operationType_, propertyName_, static_cast<int>(propertyTypeId_), customTypeId_);
            break;
        case NamePropertyOperationType::GetProperty:
            w.Write("({0}:{1}:{2})", operationType_, propertyName_, boolParam_);
            break;
        case NamePropertyOperationType::DeleteProperty:
            w.Write("({0}:{1})", operationType_, propertyName_);
            break;
        case NamePropertyOperationType::CheckExistence:
            w.Write("({0}:{1}:{2})", operationType_, propertyName_, boolParam_);
            break;
        case NamePropertyOperationType::CheckSequence:
            w.Write("({0}:{1}:{2})", operationType_, propertyName_, int64Param_);
            break;
        case NamePropertyOperationType::CheckValue:
            w.Write("({0}:{1}:{2})", operationType_, propertyName_, static_cast<int>(propertyTypeId_));
            break;
        default:
            w.Write("({0})", operationType_);
            break;
        }
    }

    std::wstring NamePropertyOperation::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }

    bool NamePropertyOperation::IsValid() const
    {
        return ValidatePropertyName();
    }

    bool NamePropertyOperation::ValidatePropertyName() const
    {
        if (propertyName_.length() < ParameterValidator::MinStringSize ||
            propertyName_.length() > ServiceModelConfig::GetConfig().MaxPropertyNameLength)
        {
            Trace.WriteWarning(
                PropertyOperationTraceType,
                "PropertyName for operation type {0} doesnt respect parameter size limits {{1} to {2}}",
                operationType_,
                ParameterValidator::MinStringSize,
                ServiceModelConfig::GetConfig().MaxPropertyNameLength);

            return false;
        }

        return true;
    }
}
