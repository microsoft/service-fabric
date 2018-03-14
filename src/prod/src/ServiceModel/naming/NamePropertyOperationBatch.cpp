// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Naming
{
    NamePropertyOperationBatch::NamePropertyOperationBatch()
        : namingUri_()
        , operations_()
    {
    }

    NamePropertyOperationBatch::NamePropertyOperationBatch(Common::NamingUri const & namingUri)
        : namingUri_(namingUri)
        , operations_()
    {
    }

    NamePropertyOperationBatch::NamePropertyOperationBatch(
        Common::NamingUri const & namingUri, 
        std::vector<NamePropertyOperation> && operations)
        : namingUri_(namingUri)
        , operations_(std::move(operations))
    {
    }

    NamePropertyOperationBatch::NamePropertyOperationBatch(NamePropertyOperationBatch && other)
        : namingUri_(move(other.namingUri_))
        , operations_(move(other.operations_))
    {
    }

    NamePropertyOperationBatch & NamePropertyOperationBatch::operator = (NamePropertyOperationBatch && other)
    {
        if (this != &other)
        {
            namingUri_ = move(other.namingUri_);
            operations_ = move(other.operations_);
        }

        return *this;
    }

    ErrorCode NamePropertyOperationBatch::FromPublicApi(std::vector<PropertyBatchOperationDescriptionSPtr> && apiBatch)
    {
        ErrorCode error;
        for (auto const & curPtr : apiBatch)
        {
            switch (curPtr->Kind)
            {
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET: 
                {
                    GetPropertyBatchOperationDescriptionSPtr curPtrDerived =
                        std::static_pointer_cast<GetPropertyBatchOperationDescription>(curPtr);
                    AddGetPropertyOperation(curPtrDerived->PropertyName, curPtrDerived->IncludeValue);
                    break;
                }
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS:
                {
                    CheckExistsPropertyBatchOperationDescriptionSPtr curPtrDerived = 
                        std::static_pointer_cast<CheckExistsPropertyBatchOperationDescription>(curPtr);
                    AddCheckExistenceOperation(curPtrDerived->PropertyName, curPtrDerived->Exists);
                    break;
                }
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE:
                {
                    CheckSequencePropertyBatchOperationDescriptionSPtr curPtrDerived = 
                        std::static_pointer_cast<CheckSequencePropertyBatchOperationDescription>(curPtr);

                    AddCheckSequenceOperation(curPtrDerived->PropertyName, curPtrDerived->SequenceNumber);
                    break;
                }
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE:
                {
                    AddDeletePropertyOperation(curPtr->PropertyName);
                    break;
                }
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE:
                {
                    CheckValuePropertyBatchOperationDescriptionSPtr curPtrDerived = 
                        std::static_pointer_cast<CheckValuePropertyBatchOperationDescription>(curPtr);

                    ByteBuffer checkValue;
                    error = curPtrDerived->Value->GetValueBytes(checkValue);
                    if (!error.IsSuccess())
                    {
                        return error;
                    }

                    AddCheckValuePropertyOperation(
                        curPtrDerived->PropertyName,
                        std::move(checkValue),
                        curPtrDerived->Value->Kind);
                    break;
                }
                case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT:
                {
                    PutPropertyBatchOperationDescriptionSPtr curPtrDerived = 
                        std::static_pointer_cast<PutPropertyBatchOperationDescription>(curPtr);

                    ByteBuffer value;
                    error = curPtrDerived->Value->GetValueBytes(value);
                    if (!error.IsSuccess())
                    {
                        return error;
                    }

                    if (StringUtility::AreEqualCaseInsensitive(curPtrDerived->CustomTypeId, L""))
                    {
                        AddPutPropertyOperation(
                            curPtrDerived->PropertyName,
                            std::move(value),
                            curPtrDerived->Value->Kind);
                    }
                    else
                    {
                        AddPutCustomPropertyOperation(
                            curPtrDerived->PropertyName,
                            std::move(value),
                            curPtrDerived->Value->Kind,
                            curPtrDerived->TakeCustomTypeId());
                    }
                    break;
                }
                default:
                {
                    return ErrorCodeValue::InvalidArgument;
                }
            }
        }
        return ErrorCode::Success();
    }

    void NamePropertyOperationBatch::AddPutPropertyOperation(
        wstring const & name, 
        vector<byte> && value, 
        ::FABRIC_PROPERTY_TYPE_ID typeId)
    {
        auto operation = NamePropertyOperation(
            NamePropertyOperationType::PutProperty, 
            name, 
            typeId,
            move(value));
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddPutCustomPropertyOperation(
        wstring const & name, 
        vector<byte> && value, 
        ::FABRIC_PROPERTY_TYPE_ID typeId,
        wstring && customTypeId)
    {
        auto operation = NamePropertyOperation(
            NamePropertyOperationType::PutCustomProperty, 
            name, 
            typeId, 
            move(value),
            move(customTypeId));
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddDeletePropertyOperation(wstring const & name)
    {
        auto operation = NamePropertyOperation(NamePropertyOperationType::DeleteProperty, name);
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddCheckExistenceOperation(wstring const & name, bool expected)
    {
        auto operation = NamePropertyOperation(NamePropertyOperationType::CheckExistence, name);
        operation.BoolParam = expected;
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddCheckSequenceOperation(wstring const & name, _int64 sequenceNumber)
    {
        auto operation = NamePropertyOperation(NamePropertyOperationType::CheckSequence, name);
        operation.Int64Param = sequenceNumber;
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddGetPropertyOperation(wstring const & name, bool includeValue)
    {
        auto operation = NamePropertyOperation(NamePropertyOperationType::GetProperty, name);
        operation.BoolParam = includeValue;
        operations_.push_back(move(operation));
    }

    void NamePropertyOperationBatch::AddCheckValuePropertyOperation(
        wstring const & name, 
        vector<byte> && value, 
        ::FABRIC_PROPERTY_TYPE_ID typeId)
    {
        auto operation = NamePropertyOperation(
            NamePropertyOperationType::CheckValue, 
            name, 
            typeId,
            move(value));
        operations_.push_back(move(operation));
    }

    bool NamePropertyOperationBatch::IsReadonly() const
    {
        for (auto iter = operations_.cbegin(); iter != operations_.cend(); ++ iter)
        {
            if (iter->IsWrite)
            {
                return false;
            }
        }

        return true;
    }

    bool NamePropertyOperationBatch::IsValid() const
    {
        bool isSystemApplicationName = false;
        if (namingUri_.ToString() == *ServiceModel::SystemServiceApplicationNameHelper::SystemServiceApplicationName)
        {
            isSystemApplicationName = true;
        }

        for (auto iter = operations_.cbegin(); iter != operations_.cend(); ++ iter)
        {
            if (isSystemApplicationName && iter->IsWrite)
            {
                return false;
            }

            if (!iter->IsValid())
            {
                return false;
            }
        }

        return true;
    }

    void NamePropertyOperationBatch::WriteToEtw(uint16 contextSequenceId) const
    {
        ServiceModel::ServiceModelEventSource::Trace->NamePropertyBatch(
            contextSequenceId,
            namingUri_.Name,
            operations_);
    }
                
    void NamePropertyOperationBatch::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "(Name=" << namingUri_;
        if (!operations_.empty())
        {
            w << ", ops=(" << operations_ << ")";
        }

        w << ")";
    }

    std::wstring NamePropertyOperationBatch::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
