// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

TracePointPayload::TracePointPayload()
{
    messageId_ = 0;
    fullyQualifiedPipeName_ = L"";;
    nodeId_ = L"";
    action_ = L"GenericAction";
}

vector<wstring> TracePointPayload::split(const wstring &s, wstring const& delimiters)
{
    vector<wstring> tokens;
    wstring str = s;
    set<wchar_t> delims;

    for (size_t x = 0; x < delimiters.size(); x ++)
    {
        delims.insert(delimiters[x]);
    }

    for (size_t x = 0; x < str.size(); x ++)
    {
        if (delims.find(str[x]) != delims.end())
        {
            str[x] = L' ';
        }
    }

    wistringstream inputStream(str, istringstream::in);
    wstring token;
 
    while(inputStream >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

void TracePointPayload::Parse(std::wstring const & message)
{
    message_ = message;
    wstring delimiters = L":[]";

    bool extractedMessageId = false;
    bool extractedPipeName = false;
    bool extractedNodeId = false;
    bool extractedAction = false;
    // Parse the string coming from the managed tracepoint client.  Extract the fields into the appropriate members.
    vector<wstring> tokens = split(message_, delimiters);

    for(vector<wstring>::iterator i = tokens.begin(); i != tokens.end(); i ++)
    {
        wstring token = (*i);
        if(!extractedMessageId)
        {
            wstringstream stringStream(token);
            stringStream >> messageId_;
            extractedMessageId = true;
        }
        else if(!extractedPipeName)
        {
            fullyQualifiedPipeName_ = wstring(token);
            extractedPipeName = true;
        }
        else if(!extractedNodeId)
        {
            nodeId_ = wstring(token);
            extractedNodeId = true;
        }
        else if(!extractedAction)
        {
            action_ = wstring(token);
            extractedAction = true;
            hasAction_ = true;
        }
    }
}

USHORT TracePointPayload::GetMessageId() const
{
    return messageId_;
}

wstring TracePointPayload::GetFullyQualifiedPipeName() const
{
    return fullyQualifiedPipeName_;
}

wstring TracePointPayload::GetNodeId() const
{
    return nodeId_;
}

wstring TracePointPayload::GetAction() const
{
    return action_;
}

bool TracePointPayload::HasAction() const
{
    return hasAction_;
}
