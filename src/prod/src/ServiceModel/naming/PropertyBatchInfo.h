// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyBatchInfo : public Common::IFabricJsonSerializable
    {
    public:
        PropertyBatchInfo();

        PropertyBatchInfo(NamePropertyOperationBatchResult && batchResult);

        ~PropertyBatchInfo();

        __declspec(property(get=get_Kind)) PropertyBatchInfoType::Enum const & Kind;
        __declspec(property(get=get_ErrorMessage)) std::wstring const & ErrorMessage;
        __declspec(property(get=get_OperationIndex)) ULONG const & OperationIndex;
        __declspec(property(get=get_Properties)) std::map<std::wstring, PropertyInfo> const & Properties;

        PropertyBatchInfoType::Enum const & get_Kind() const { return kind_; }
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }
        ULONG const & get_OperationIndex() const { return operationIndex_; }
        std::map<std::wstring, PropertyInfo> const & get_Properties() const { return properties_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ErrorMessage, errorMessage_, kind_ == PropertyBatchInfoType::Failed)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::OperationIndex, operationIndex_, kind_ == PropertyBatchInfoType::Failed)
            SERIALIZABLE_PROPERTY_SIMPLE_MAP_IF(ServiceModel::Constants::Properties, properties_, kind_ == PropertyBatchInfoType::Successful)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        PropertyBatchInfoType::Enum kind_;
        std::wstring errorMessage_;
        ULONG operationIndex_;
        std::map<std::wstring, PropertyInfo> properties_;
    };

}
