// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceManifestGenerator
    {
    public:
        TraceManifestGenerator();

        StringWriterA & GetEventsWriter()
        {
            return eventsWriter_;
        }

        StringWriterA & GetMapWriter()
        {
            return mapsWriter_;
        }

        StringWriterA & GetTemplateWriter()
        {
            return templatesWriter_;
        }

        StringWriterA & GetTaskWriter()
        {
            return tasksWriter_;
        }

        StringWriterA & GetKeywordWriter()
        {
            return keywordWriter_;
        }

        std::string StringResource(StringLiteral original);

        std::string StringResource(std::string const & original);

        void Write(FileWriter & writer, Guid guid, std::string const & name, std::string const & message, std::string const & symbol, std::string const & targetFile);

    private:
        void WriteContents(FileWriter & writer);
        void WriteChannels(FileWriter & writer, bool isForLease);
        void WriteResources(FileWriter & writer);

        std::string events_;
        std::string templates_;
        std::string maps_;
        std::string tasks_;
        std::string keyword_;

        StringWriterA eventsWriter_;
        StringWriterA templatesWriter_;
        StringWriterA mapsWriter_;
        StringWriterA tasksWriter_;
        StringWriterA keywordWriter_;

        std::map<size_t, std::map<std::string, size_t>> stringTable_;
    };
}
