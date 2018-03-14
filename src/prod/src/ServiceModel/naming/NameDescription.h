// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NameDescription : public Common::IFabricJsonSerializable
    {
    public:
        NameDescription()
            : name_()
        {
        }

        explicit NameDescription(std::wstring const & name)
            : name_(name)
        {
        }

        ~NameDescription() {};
        
        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return name_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, name_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
    };
}
