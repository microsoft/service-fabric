// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    struct OperationResult
    {
        OperationResult(std::wstring const& value, bool isError)
            : Type(isError ? L"Error" : L"Literal"), Literal(value)
        {
        }

        OperationResult(bool value)
            : Type(L"NonLiteral"), NonLiteral(value)
        {
        }

        OperationResult(OperationResult const & value)
            :Type(value.Type), NonLiteral(value.NonLiteral), Literal(value.Literal)
        {
        }

        
        bool IsError() { return Type.compare(L"Error") == 0; }
        bool IsNonLiteral() { return Type.compare(L"NonLiteral") == 0; }

        bool NonLiteral;
        std::wstring Literal;

    private:
        std::wstring Type;
    };
}
