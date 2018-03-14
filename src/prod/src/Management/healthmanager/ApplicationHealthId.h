// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ApplicationHealthId : public Serialization::FabricSerializable
        {
        public:
            ApplicationHealthId();

            explicit ApplicationHealthId(
                std::wstring const & applicationName);

            explicit ApplicationHealthId(
                std::wstring && applicationName);

            ApplicationHealthId(ApplicationHealthId const & other);
            ApplicationHealthId & operator = (ApplicationHealthId const & other);

            ApplicationHealthId(ApplicationHealthId && other);
            ApplicationHealthId & operator = (ApplicationHealthId && other);
            
            ~ApplicationHealthId();
            
            __declspec (property(get=get_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }
            
            bool operator == (ApplicationHealthId const & other) const;
            bool operator != (ApplicationHealthId const & other) const;
            bool operator < (ApplicationHealthId const & other) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_01(applicationName_);
            
        private:
            std::wstring applicationName_;
        };
    }
}
