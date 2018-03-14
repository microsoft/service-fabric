// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef struct NamedPropertyData
    {
        std::wstring PropertyName;
        ::FABRIC_PROPERTY_TYPE_ID PropertyType;
        std::vector<byte> Bytes;
        std::wstring CustomTypeId;
    } NamedPropertyData;

    // Property Enumeration
    typedef std::map<std::wstring, NamedPropertyData> NamedPropertyMap;
    typedef std::map<Common::NamingUri, NamedPropertyMap> PropertyStateMap;
    typedef std::map<Common::NamingUri, Common::ComPointer<IFabricPropertyEnumerationResult>> PropertyEnumerationTokenMap;

    // Name Enumeration
    typedef std::map<Common::NamingUri, Common::ComPointer<IFabricNameEnumerationResult>> NameEnumerationTokenMap;

    class FabricTestNamingState
    {
        DENY_COPY(FabricTestNamingState)

    public:     
        FabricTestNamingState() { }
        virtual ~FabricTestNamingState() { }

        // Property Enumeration
        void AddProperty(
            Common::NamingUri const &,
            std::wstring const &, 
            ::FABRIC_PROPERTY_TYPE_ID, 
            std::vector<byte> const &,
            std::wstring const & customTypeId);
        void RemoveProperty(
            Common::NamingUri const &, 
            std::wstring const &);

        void AddProperties(
            Common::NamingUri const &,
            std::vector<NamedPropertyData> const & properties);

        void AddPropertyEnumerationResult(
            Common::NamingUri const &, 
            std::wstring const &, 
            ::FABRIC_PROPERTY_TYPE_ID, 
            std::vector<byte> const &,
            std::wstring const & customTypeId);
        bool VerifyPropertyEnumerationResults(
            Common::NamingUri const &, 
            bool includeValues);

        void SetPropertyEnumerationToken(
            Common::NamingUri const &, 
            ComPointer<IFabricPropertyEnumerationResult> const &);
        Common::ComPointer<IFabricPropertyEnumerationResult> GetPropertyEnumerationToken(
            Common::NamingUri const &);
        
        void ClearPropertyEnumerationResults(Common::NamingUri const &);

        // Name Enumeration
        void SetNameEnumerationToken(
            Common::NamingUri const & parent, 
            ComPointer<IFabricNameEnumerationResult> const &);
        Common::ComPointer<IFabricNameEnumerationResult> GetNameEnumerationToken(
            Common::NamingUri const & parent);
        
        void ClearNameEnumerationResults(Common::NamingUri const &);

    private:
        void AddPropertyToMap(
            Common::NamingUri const &, 
            std::wstring const &, 
            ::FABRIC_PROPERTY_TYPE_ID, 
            std::vector<byte> const &, 
            std::wstring const & customTypeId, 
            __in PropertyStateMap &);

        std::wstring ToString(NamedPropertyData const &);
        bool AreEqual(NamedPropertyData const &, NamedPropertyData const &, bool includeValues);

        PropertyStateMap propertyState_;
        PropertyStateMap propertyEnumerationResults_;
        PropertyEnumerationTokenMap propertyEnumerationTokens_;

        NameEnumerationTokenMap nameEnumerationTokens_;

        Common::RwLock lock_;
    };
}
