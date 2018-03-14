// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class AttributeList
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(AttributeList)
        DEFAULT_COPY_ASSIGNMENT(AttributeList)
    
    public:
        AttributeList();
        
        explicit AttributeList(
          std::map<std::wstring, std::wstring> const& attributes);

        AttributeList(AttributeList && other);
        AttributeList & operator = (AttributeList && other);

        _declspec(property(get=get_Attributes)) std::map<std::wstring, std::wstring> const & Attributes;
        std::map<std::wstring, std::wstring> const & get_Attributes() const { return attributes_; }

        void AddAttribute(
            std::wstring const & key,
            std::wstring const & value);

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        
        Common::ErrorCode FromPublicApi(FABRIC_STRING_PAIR_LIST const & attributeList);
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_STRING_PAIR_LIST & attributes) const;

        FABRIC_FIELDS_01(attributes_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(attributes_)
        END_DYNAMIC_SIZE_ESTIMATION()
        
    private:
        std::map<std::wstring, std::wstring> attributes_;
    };
}
