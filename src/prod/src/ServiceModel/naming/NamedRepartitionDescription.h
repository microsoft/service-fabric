// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamedRepartitionDescription : public RepartitionDescription
    {
        DEFAULT_COPY_CONSTRUCTOR(NamedRepartitionDescription)
        DEFAULT_COPY_ASSIGNMENT(NamedRepartitionDescription)
        
    public:
        NamedRepartitionDescription();
        NamedRepartitionDescription(std::vector<std::wstring> && toAdd, std::vector<std::wstring> && toRemove);
        NamedRepartitionDescription(std::vector<std::wstring> const & toAdd, std::vector<std::wstring> const & toRemove);
        NamedRepartitionDescription(NamedRepartitionDescription &&);

        __declspec(property(get=get_NamesToAdd)) std::vector<std::wstring> const & NamesToAdd;
        std::vector<std::wstring> const & get_NamesToAdd() const { return namesToAdd_; }

        __declspec(property(get=get_NamesToRemove)) std::vector<std::wstring> const & NamesToRemove;
        std::vector<std::wstring> const & get_NamesToRemove() const { return namesToRemove_; }

        void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const override;

        FABRIC_FIELDS_02(
            namesToAdd_,
            namesToRemove_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NamesToAdd, namesToAdd_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NamesToRemove, namesToRemove_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:

        void OnWriteToEtw(uint16 contextSequenceId) const override;

    private:
        std::vector<std::wstring> namesToAdd_;
        std::vector<std::wstring> namesToRemove_;
    };
}
