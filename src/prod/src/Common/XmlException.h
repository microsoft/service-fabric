// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    // Exception type used in XML reading, writing and parsing. 
    // Xml reading, writing and parsing code is greatly simplified by 
    // the use of exception. The caller of XmlReader and XmlWriter
    // methods should catch XmlException and return the error code contained
    // in them to the rest of the system.
    class XmlException
    {
    public:
        XmlException(ErrorCode error) :
            error_(error)
        {
        }

        __declspec (property(get=get_Error)) ErrorCode const & Error;
        ErrorCode const & get_Error() const { return this->error_; }

    private:
        ErrorCode error_;
    };
}
