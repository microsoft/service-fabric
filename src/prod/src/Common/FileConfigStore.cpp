// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/FileConfigStore.h"
#include "Common/File.h"
#include "Common/Parse.h"
#include "Common/AssertWF.h"

namespace 
{
    using Common::StringWriter;
    using std::wstring;

    // Private methods used by the FileConfigStore implementation
    // Finds the first non-encoded appearance of ch;
    // if not found, wstring::npos is returned
    size_t GetFirstNonEncodedPosition(wstring const & line, wchar_t ch, size_t startIndex = 0)
    {
        static const wchar_t encoder = L'\\';

        size_t index = 0;
        bool done = false;
        while(!done)
        {
            index = line.find_first_of(ch, startIndex);
            if(index == wstring::npos)
            {
                done = true;
            }
            else
            {
                startIndex = index + 1;
                int numberOfEncoders = 0;
                while(index > 0 && line[index - 1] == encoder)
                {
                    ++numberOfEncoders;
                    --index;
                }

                // an even number of encoders preceeding ch means that ch is encoded
                // EG. \;, \\\; 
                if(numberOfEncoders % 2 == 0)
                {
                    // The character is not encoded
                    done = true;
                }
            }
        }

        return index;
    }

    // Remove everything after an encoded ;
    void RemoveComment(wstring & str)
    {
        static const wchar_t commentDelim = L';';
        size_t index = GetFirstNonEncodedPosition(str, commentDelim);
        if (index != wstring::npos)
        {
            str.resize(index);
        }
    }

    wstring GetNextLine(wstring const & text, size_t & pos)
    {
        static wstring lineSeparator = L"\n\r";
        size_t start = text.find_first_not_of(lineSeparator, pos);
        if(start == wstring::npos)
        {
            return wstring();
        }

        pos = text.find_first_of(lineSeparator, start);
        if(pos == wstring::npos)
        {
            // Return all the text since start
            return text.substr(start);
        }
        else
        {
            return text.substr(start, pos - start);
        }
    }

    // Check if the crtLine is a section name (a name defined inside [])
    bool IsSectionName(wstring const & line)
    {
        static const wchar_t sectionStart = L'[';
        static const wchar_t sectionEnd = L']';

        if(line[0] == sectionStart)
        {
            // [ marks a section name; 
            // if the file is correct, it should contain just one ], at the end of the string
            // and no more [
            ASSERT_IF(line.size() - 1 != line.find_first_of(sectionEnd), "The only allowed ] in section name is at the end");
            ASSERT_IF(wstring::npos != line.find_first_of(sectionStart, 1), "The only allowed [ in section name is at the beginning");

            return true;
        }

        // The line is not a section name
        return false;
    }
} // end annonymous namespace

namespace Common
{
    using Common::Environment;
    using Common::File;
    using Common::Path;
    using Common::TimeSpan;
    using Common::StringCollection;
    using Common::StringUtility;
    using std::pair;
    using std::string;
    using std::wstring;

    FileConfigStore::FileConfigStore(wstring const & fileName, bool readFromString)
    {
        if(readFromString)
        {
            Parse(fileName);
            return;
        }

        wstring fileNameLocal = fileName;
        if (!File::Exists(fileNameLocal))
        {
            ConfigEventSource::Events.FileConfigStoreFileNotFound(fileNameLocal);
            return;
        }
        
        File fileReader;
        auto error = fileReader.TryOpen(fileNameLocal, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            ConfigEventSource::Events.FileConfigStoreFileReadError(fileNameLocal);
            return;
        }

        ConfigEventSource::Events.FileConfigStoreFileReadSuccess(fileNameLocal);
        // TODO: if files are large, read line by line
        // instead of all file content
        int64 fileSize = fileReader.size();
        // For now, we don't expect the files to be large.
        ASSERT_IF(fileSize > 100000, "File size is {0}", fileSize);

        size_t size = static_cast<size_t>(fileSize);

        // TODO: use streams instead of reading bytes,
        // because they know how to deal with Unicode/ASCII
        // For now, until the build team fully integrates VC10, 
        // we do not have access to �locale�, �regex� and �stream� parts of the VC10 standard library
        string configText;
        configText.resize(size);
        fileReader.Read(&configText[0], static_cast<int>(size));
        fileReader.Close();

        wstring configTextW;
        StringWriter(configTextW).Write(configText);
        Parse(configTextW);
    }
     
    void FileConfigStore::Parse(wstring const & configText)
    {
        wstring crtLine;
        wstring crtSectionName;
        SectionMap crtKeyValues;
                
        size_t pos = 0;
        size_t oldPos;
        while (pos != std::wstring::npos)
        {
            oldPos = pos;
            crtLine = GetNextLine(configText, pos);
            if(oldPos == pos)
            {
                // No more lines that contain text different than "\r\n"
                break;
            }

            RemoveComment(crtLine);
            StringUtility::TrimWhitespaces(crtLine);
            if(crtLine.empty())
            {
                continue;
            }

            if(IsSectionName(crtLine))
            {
                // Add the previous section to the sections map
                AddSection(crtSectionName, crtKeyValues);
                
                // Then create a new section to hold future key/value pairs
                crtSectionName.clear();
                crtKeyValues.clear();
                ParseSectionName(crtLine, crtSectionName);                
            }
            else
            {
                ParseKeyValue(crtLine, crtKeyValues);
            }
        }

        // Write the last section to the sections map
        AddSection(crtSectionName, crtKeyValues);
    }
        
    FileConfigStore::~FileConfigStore()
    {
    }

    void FileConfigStore::AddSection(wstring const & crtSectionName, SectionMap const & crtKeyValues)
    {
        if(!crtKeyValues.empty())
        {
            ASSERT_IF(crtSectionName.empty(), "Keys/Values defined outside a section name");
            sections[crtSectionName] = crtKeyValues;
        }
    }
        
    void FileConfigStore::ParseSectionName(wstring const & line, wstring & crtSectionName)
    {
        // Remove [ and ] from the beginning and end of string
        crtSectionName = line.substr(1, line.size() - 2);
        StringUtility::TrimWhitespaces(crtSectionName);
    }

    void FileConfigStore::ParseKeyValue(wstring const & line, SectionMap & crtKeyValues)
    {
        static const wchar_t keyValueSeparator = L'=';

        size_t index = GetFirstNonEncodedPosition(line, keyValueSeparator, 0);
        ASSERT_IF(index == wstring::npos, "key/value not separated by =");

        // Verify that there is no other non-encoded = in the string
        ASSERT_IF(
            GetFirstNonEncodedPosition(line, keyValueSeparator, index + 1) != wstring::npos,
            "Line tried to define multiple Key/value pairs");
        
        ASSERT_IF(index == 0, "empty key name is not allowed");

        wstring key = line.substr(0, index);
        StringUtility::TrimWhitespaces(key);
        wstring value = line.substr(index + 1);
        StringUtility::TrimWhitespaces(value);
        StringUtility::Replace<wstring>(value, L"\\;", L";");
        StringUtility::Replace<wstring>(value, L"\\=", L"=");
        crtKeyValues[key] = value;        
    }

    bool FileConfigStore::GetSectionMap(wstring const & section, SectionMap & keyValuesMap) const
    {
        auto matchSectionNameLambda = [section](pair<wstring, SectionMap> const & mapPair) -> bool
        {
            return StringUtility::AreEqualCaseInsensitive(mapPair.first, section);
        };

        DocumentMapConstIterator sectionIter = find_if(sections.begin(), sections.end(), matchSectionNameLambda);
        if(sections.end() == sectionIter)
        {
            // The config section is not defined
            return false;
        }
        else
        {
            keyValuesMap = sectionIter->second;
            return true;
        }
    }

    bool FileConfigStore::GetKey(wstring const & key, SectionMap const & keyValueMap, wstring & value) const
    {
        auto matchKeyLambda = [key](pair<wstring, wstring> const & mapPair) -> bool
        {   
            return StringUtility::AreEqualCaseInsensitive(mapPair.first, key);
        };

        SectionMapConstIterator keyValueIter = find_if(keyValueMap.begin(), keyValueMap.end(), matchKeyLambda);
        if(keyValueMap.end() == keyValueIter)
        {
            // The key doesn't exist in this section 
            return false;
        }
        else
        {
            value = keyValueIter->second;
            return true;
        }
    }

    std::wstring FileConfigStore::ReadString(
        wstring const & section, 
        wstring const & key,
        __out bool & isEncrypted) const
    {
        isEncrypted = false;

        std::wstring result;
        SectionMap keyValueMap;
        if(GetSectionMap(section, keyValueMap))
        {
            if(GetKey(key, keyValueMap, result))
            {
                StringUtility::Replace<std::wstring>(result, L"\\=", L"="); // '=' had to be escaped because it seperates key and value
                StringUtility::Replace<std::wstring>(result, L"\\;", L";"); // ';' had to be escaped because it precedes comment text
                return result;
            }
        }

        // The section or the key were not found
        return L"";
    }
     
    void FileConfigStore::GetSections(
        Common::StringCollection & sectionNames, 
        wstring const & partialName) const
    {
        bool allNames = partialName.empty();
        for(DocumentMapConstIterator sectionIter = sections.begin(); sectionIter != sections.end(); ++sectionIter)
        {
            if(allNames || StringUtility::ContainsCaseInsensitive(sectionIter->first, partialName))
            {
                sectionNames.push_back(sectionIter->first);
            }
        }
    }

    void FileConfigStore::GetKeys(
        wstring const & section,
        Common::StringCollection & keyNames, 
        wstring const & partialName) const
    {
        SectionMap sectionMap;
        if(GetSectionMap(section, sectionMap))
        {
            // The section exists, so get all the keys inside it
            bool allNames = partialName.empty();
            for(SectionMapConstIterator keyValuesIter = sectionMap.begin(); keyValuesIter != sectionMap.end(); ++keyValuesIter)
            {
                if(allNames || StringUtility::ContainsCaseInsensitive(keyValuesIter->first, partialName))
                {
                    keyNames.push_back(keyValuesIter->first);
                }
            }
        }
    }
} // end namespace Common
