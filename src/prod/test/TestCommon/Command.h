// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class Command
    {
    public:
        __declspec(property(get = get_LineNumber)) int LineNumber;
        int get_LineNumber() const { return lineNumber_; }

        __declspec(property(get = get_Text)) std::wstring const & Text;
        std::wstring const & get_Text() const { return text_; }

        __declspec(property(get = get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return text_.empty(); }

        static Command CreateCommandFromSourceFile(std::wstring && commandText, int lineNumber)
        {
            return Command(std::move(commandText), lineNumber);
        }

        static Command CreateCommand(std::wstring && commandText)
        {
            return Command(std::move(commandText), -1);
        }

        static Command CreateCommand(std::wstring const & commandText)
        {
            return Command(commandText, -1);
        }

        static Command CreateEmptyCommand()
        {
            return Command(std::wstring(L""), -1);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            if (lineNumber_ == -1)
            {
                w.Write(text_);
            }
            else
            {
                w.Write("[{0}] {1}", lineNumber_, text_);
            }
        }

    private:
        Command(std::wstring && commandText, int lineNumber)
            : lineNumber_(lineNumber),
              text_(std::move(commandText))
        {
            Common::StringUtility::TrimWhitespaces(text_);
        }

        Command(std::wstring const & commandText, int lineNumber)
            : lineNumber_(lineNumber),
              text_(commandText)
        {
            Common::StringUtility::TrimWhitespaces(text_);
        }

        int lineNumber_;
        std::wstring text_;
    };
}
