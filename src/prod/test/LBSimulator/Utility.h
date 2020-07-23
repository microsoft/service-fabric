// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class Utility
    {
        DENY_COPY(Utility);

    public:
        static std::wstring const ParamDelimiter;
        static std::wstring const FieldDelimiter;
        static std::wstring const ItemDelimiter;
        static std::wstring const PairDelimiter;
        static std::wstring const KeyValueDelimiter;
        static std::wstring const ConfigDelimiter;
        static std::wstring const CommentInit;
        static std::wstring const SectionConfig;
        static std::wstring const SectionStart;
        static std::wstring const SectionEnd;
        static std::wstring const VectorStartDelimiter;
        static std::wstring const VectorDelimiter;

        static std::wstring const EmptyCharacter;

        static Common::StringLiteral const TraceSource;

        static bool Bool_Parse(std::wstring const & input);
        static Common::StringCollection Split(std::wstring const& input, std::wstring const& delimiter);
        static Common::StringCollection Split(std::wstring const& input, std::wstring const& delimiter, std::wstring const& emptyChar);
        static Common::StringCollection SplitWithParentheses(std::wstring const& input,
            std::wstring const& delimiter, std::wstring const& emptyChar);

        static void LoadFile(std::wstring const & fileName, std::wstring & fileTextW);
        static void Tokenize(std::wstring & text, std::vector<std::pair<std::wstring, std::wstring>> & tokens);
        static void Label(std::vector<std::pair<std::wstring, std::wstring>> & tokens,
            std::map<std::wstring, std::wstring> & labelledEntries, std::vector<std::wstring> labels);
        static std::wstring ReadLine(std::wstring const & text, size_t & pos);
        static void UpdateConfigValue(std::map<std::wstring, std::wstring> &, Reliability::LoadBalancingComponent::PLBConfig &);
        static std::wstring GetConfigValue(std::wstring const & configName, Reliability::LoadBalancingComponent::PLBConfig & config);
        static void ParseConfigSection(std::wstring const & fileTextW, size_t & pos,
            Reliability::LoadBalancingComponent::PLBConfig & config);
        static void ReadUint64(const std::wstring & string, uint64 & integer);
    };
}
