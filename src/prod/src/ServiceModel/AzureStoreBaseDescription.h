// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct AzureStoreBaseDescription
    {
    public:
        AzureStoreBaseDescription();
        AzureStoreBaseDescription(AzureStoreBaseDescription const & other);
        AzureStoreBaseDescription(AzureStoreBaseDescription && other);

        AzureStoreBaseDescription const & operator = (AzureStoreBaseDescription const & other);
        AzureStoreBaseDescription const & operator = (AzureStoreBaseDescription && other);

        bool operator == (AzureStoreBaseDescription const & other) const;
        bool operator != (AzureStoreBaseDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void clear();

    public:
        std::wstring IsEnabled;
        std::wstring ConnectionString;
        std::wstring ConnectionStringIsEncrypted;
        std::wstring UploadIntervalInMinutes;
        std::wstring DataDeletionAgeInDays;
        ParametersDescription Parameters;

    protected:
        void ReadFromXmlWorker(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteXmlWorker(Common::XmlWriterUPtr const & xmlWriter);
    };
}
