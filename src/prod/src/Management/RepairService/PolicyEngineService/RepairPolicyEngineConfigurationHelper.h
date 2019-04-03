// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class RepairPolicyEngineConfigurationHelper
    {
	public:
		RepairPolicyEngineConfigurationHelper();

		template <class F, class T>
        static T Convert(F value);

        template<>
        static Common::TimeSpan Convert(double value)
        {
            return Common::TimeSpan::FromSeconds(value);
        }

        template<> 
		static uint Convert(uint value)
        {
            return value;
        }

        template<> 
        static bool Convert(bool value)
        {
            return value;
        }

        template<class F>
        static bool IsLessThanZero(F value);

        template<>
        static bool IsLessThanZero(uint value)
        {
            UNREFERENCED_PARAMETER(value);
            return false;
        }

        template<>
        static bool IsLessThanZero(double value)
        {
            return value < 0;
        }

        template<>
        static bool IsLessThanZero(bool value)
        {
            UNREFERENCED_PARAMETER(value);
            return false;
        }

        template <class F, class T>
        static bool TryParse(LPCWSTR parameterName, LPCWSTR valueToParse,  T& parsedValue)
        {
            CODING_ERROR_ASSERT(parameterName != nullptr);
            std::wstring valueToParseW;
            std::wstring parameterNameW;
            
            try
            {
                parameterNameW = std::wstring(parameterName);

                if (valueToParse == nullptr)
                {
                    Trace.WriteError(ComponentName, "Configuration parameter '{0}' has empty or null value. Default value will be used.", parameterNameW);
                    return false;
                }

                valueToParseW = std::wstring(valueToParse);
                typename F outValue = 0;

                if (valueToParseW.length() == 0 ||
                    !Config::TryParse<F>(outValue, valueToParseW) ||
                    IsLessThanZero<F>(outValue))
                {
                    Trace.WriteError(ComponentName, "Configuration parameter '{0}' has invalid value '{1}'. Default value will be used.", parameterNameW, valueToParseW);
                    return false;
                }

                parsedValue = Convert<F, T>(outValue);

                Trace.WriteNoise(ComponentName, "Configuration parameter '{0}' has default value being overridden from file. New value '{1}'", parameterNameW, outValue);
            }
            catch (exception const& e)
            {
                Trace.WriteError(ComponentName, "Configuration parameter '{0}' parsing of value '{1}' failed with exception:{2}", parameterNameW, valueToParseW, e.what());
                return false;
            }

            return true;
        };
	};
}
