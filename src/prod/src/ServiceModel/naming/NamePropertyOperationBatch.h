// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Naming
{
    class NamePropertyOperationBatch : public ServiceModel::ClientServerMessageBody
    {
        DENY_COPY(NamePropertyOperationBatch)

    public:
        NamePropertyOperationBatch();

        explicit NamePropertyOperationBatch(Common::NamingUri const & namingUri);

        NamePropertyOperationBatch(
            Common::NamingUri const & namingUri, 
            std::vector<NamePropertyOperation> && operations);

        NamePropertyOperationBatch(NamePropertyOperationBatch && other);
        NamePropertyOperationBatch & operator = (NamePropertyOperationBatch && other);
                
        __declspec(property(get=get_NamingUri)) Common::NamingUri const & NamingUri;
        Common::NamingUri const & get_NamingUri() const { return namingUri_; }
        
        __declspec(property(get=get_Operations)) std::vector<NamePropertyOperation> const & Operations;
        std::vector<NamePropertyOperation> & get_Operations() { return operations_; }

        Common::ErrorCode FromPublicApi(std::vector<PropertyBatchOperationDescriptionSPtr> && apiBatch);

        void AddPutPropertyOperation(
            std::wstring const & name, 
            std::vector<byte> && value, 
            ::FABRIC_PROPERTY_TYPE_ID typeId);

        void AddPutCustomPropertyOperation(
            std::wstring const & name, 
            std::vector<byte> && value, 
            ::FABRIC_PROPERTY_TYPE_ID typeId, 
            std::wstring && customTypeId);

        void AddDeletePropertyOperation(std::wstring const &);
        void AddCheckExistenceOperation(std::wstring const &, bool exists);
        void AddCheckSequenceOperation(std::wstring const &, _int64);
        void AddGetPropertyOperation(std::wstring const &, bool includeValue = true);

        void AddCheckValuePropertyOperation(
            std::wstring const & name, 
            std::vector<byte> && value, 
            ::FABRIC_PROPERTY_TYPE_ID typeId);

        bool IsReadonly() const;
        bool IsValid() const;

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_02(namingUri_, operations_);

    protected:
        Common::NamingUri namingUri_;
        std::vector<NamePropertyOperation> operations_;
    };
}
