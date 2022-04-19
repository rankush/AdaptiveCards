// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "DefaultActionSentimentResourceDictionary.xaml.g.h"

namespace winrt::AdaptiveCards::Rendering::Uwp::implementation
{
    struct DefaultActionSentimentResourceDictionary : DefaultActionSentimentResourceDictionaryT<DefaultActionSentimentResourceDictionary>
    {
        DefaultActionSentimentResourceDictionary();
    };
}

namespace winrt::AdaptiveCards::Rendering::Uwp::factory_implementation
{
    struct DefaultActionSentimentResourceDictionary
        : DefaultActionSentimentResourceDictionaryT<DefaultActionSentimentResourceDictionary, implementation::DefaultActionSentimentResourceDictionary>
    {
    };
}
