// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "pch.h"

#include "ActionHelpers.h"
#include "AdaptiveContainerRenderer.h"
#include "AdaptiveRenderArgs.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::AdaptiveCards::Rendering::WinUI3;
using namespace ABI::AdaptiveCards::ObjectModel::WinUI3;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::UI::Xaml;
using namespace ABI::Windows::UI::Xaml::Controls;

namespace AdaptiveCards::Rendering::WinUI3
{
    HRESULT AdaptiveContainerRenderer::RuntimeClassInitialize() noexcept { return S_OK; }

    HRESULT AdaptiveContainerRenderer::Render(_In_ IAdaptiveCardElement* adaptiveCardElement,
                                              _In_ IAdaptiveRenderContext* renderContext,
                                              _In_ IAdaptiveRenderArgs* renderArgs,
                                              _COM_Outptr_ IUIElement** containerControl) noexcept
    try
    {
        ComPtr<IAdaptiveCardElement> cardElement(adaptiveCardElement);
        ComPtr<IAdaptiveContainer> adaptiveContainer;
        RETURN_IF_FAILED(cardElement.As(&adaptiveContainer));

        ComPtr<WholeItemsPanel> containerPanel;
        RETURN_IF_FAILED(MakeAndInitialize<WholeItemsPanel>(&containerPanel));

        ComPtr<IFrameworkElement> containerPanelAsFrameWorkElement;
        RETURN_IF_FAILED(containerPanel.As(&containerPanelAsFrameWorkElement));

        // Get any RTL setting set on either the current context or on this container. Any value set on the container
        // should be set on the context to apply to all children
        ComPtr<IReference<bool>> previousContextRtl;
        RETURN_IF_FAILED(renderContext->get_Rtl(&previousContextRtl));
        ComPtr<IReference<bool>> currentRtl = previousContextRtl;

        ComPtr<IReference<bool>> containerRtl;
        RETURN_IF_FAILED(adaptiveContainer->get_Rtl(&containerRtl));

        bool updatedRtl = false;
        if (containerRtl != nullptr)
        {
            currentRtl = containerRtl;
            RETURN_IF_FAILED(renderContext->put_Rtl(currentRtl.Get()));
            updatedRtl = true;
        }

        if (currentRtl)
        {
            boolean rtlValue;
            RETURN_IF_FAILED(currentRtl->get_Value(&rtlValue));

            RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_FlowDirection(rtlValue ? FlowDirection_RightToLeft :
                                                                                            FlowDirection_LeftToRight));
        }

        // Assign vertical alignment to strech so column will stretch and respect vertical content alignment
        ABI::AdaptiveCards::ObjectModel::WinUI3::HeightType containerHeightType{};
        RETURN_IF_FAILED(cardElement->get_Height(&containerHeightType));
        if (containerHeightType == ABI::AdaptiveCards::ObjectModel::WinUI3::HeightType::Auto)
        {
            RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_VerticalAlignment(ABI::Windows::UI::Xaml::VerticalAlignment_Stretch));
        }

        ComPtr<IAdaptiveContainerBase> containerAsContainerBase;
        RETURN_IF_FAILED(adaptiveContainer.As(&containerAsContainerBase));

        UINT32 containerMinHeight{};
        RETURN_IF_FAILED(containerAsContainerBase->get_MinHeight(&containerMinHeight));
        if (containerMinHeight > 0)
        {
            RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_MinHeight(containerMinHeight));
        }

        ComPtr<IBorder> containerBorder =
            XamlHelpers::CreateABIClass<IBorder>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Border));

        ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle;
        //winrt::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle;
        RETURN_IF_FAILED(XamlHelpers::HandleStylingAndPadding(
            containerAsContainerBase.Get(), containerBorder.Get(), renderContext, renderArgs, &containerStyle));

        ComPtr<IFrameworkElement> parentElement;
        RETURN_IF_FAILED(renderArgs->get_ParentElement(&parentElement));
        auto newRenderArgs = winrt::make<rtrender::implementation::AdaptiveRenderArgs>(containerStyle, parentElement.Get(), renderArgs);

        ComPtr<IPanel> containerPanelAsPanel;
        RETURN_IF_FAILED(containerPanel.As(&containerPanelAsPanel));
        ComPtr<IVector<IAdaptiveCardElement*>> childItems;
        RETURN_IF_FAILED(adaptiveContainer->get_Items(&childItems));
        /*RETURN_IF_FAILED(XamlBuilder::BuildPanelChildren(
            childItems.Get(), containerPanelAsPanel.Get(), renderContext, newRenderArgs.Get(), [](IUIElement*) {}));*/
        RETURN_IF_FAILED(XamlBuilder::BuildPanelChildren(
            childItems.Get(), containerPanelAsPanel.Get(), renderContext, renderArgs, [](IUIElement*) {}));

        // If we changed the context's rtl setting, set it back after rendering the children
        if (updatedRtl)
        {
            RETURN_IF_FAILED(renderContext->put_Rtl(previousContextRtl.Get()));
        }

        ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::VerticalContentAlignment>> verticalContentAlignmentReference;
        RETURN_IF_FAILED(adaptiveContainer->get_VerticalContentAlignment(&verticalContentAlignmentReference));

        ABI::AdaptiveCards::ObjectModel::WinUI3::VerticalContentAlignment verticalContentAlignment =
            ABI::AdaptiveCards::ObjectModel::WinUI3::VerticalContentAlignment::Top;
        if (verticalContentAlignmentReference != nullptr)
        {
            verticalContentAlignmentReference->get_Value(&verticalContentAlignment);
        }

        XamlHelpers::SetVerticalContentAlignmentToChildren(containerPanel.Get(), verticalContentAlignment);

        // Check if backgroundImage defined
        ComPtr<IAdaptiveBackgroundImage> backgroundImage;
        BOOL backgroundImageIsValid;
        RETURN_IF_FAILED(adaptiveContainer->get_BackgroundImage(&backgroundImage));
        RETURN_IF_FAILED(IsBackgroundImageValid(backgroundImage.Get(), &backgroundImageIsValid));
        if (backgroundImageIsValid)
        {
            ComPtr<IGrid> rootElement =
                XamlHelpers::CreateABIClass<IGrid>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Grid));
            ComPtr<IPanel> rootAsPanel;
            RETURN_IF_FAILED(rootElement.As(&rootAsPanel));

            /*XamlHelpers::ApplyBackgroundToRoot(rootAsPanel.Get(), backgroundImage.Get(), renderContext, newRenderArgs.Get());*/
            XamlHelpers::ApplyBackgroundToRoot(rootAsPanel.Get(), backgroundImage.Get(), renderContext, renderArgs);

            // Add rootElement to containerBorder
            ComPtr<IUIElement> rootAsUIElement;
            RETURN_IF_FAILED(rootElement.As(&rootAsUIElement));
            RETURN_IF_FAILED(containerBorder->put_Child(rootAsUIElement.Get()));

            // Add containerPanel to rootElement
            ComPtr<IFrameworkElement> containerPanelAsFElement;
            RETURN_IF_FAILED(containerPanel.As(&containerPanelAsFElement));
            XamlHelpers::AppendXamlElementToPanel(containerPanelAsFElement.Get(), rootAsPanel.Get(), containerHeightType);
        }
        else
        {
            // instead, directly add containerPanel to containerBorder
            ComPtr<IUIElement> containerPanelAsUIElement;
            RETURN_IF_FAILED(containerPanel.As(&containerPanelAsUIElement));
            RETURN_IF_FAILED(containerBorder->put_Child(containerPanelAsUIElement.Get()));
        }

        RETURN_IF_FAILED(XamlHelpers::SetStyleFromResourceDictionary(renderContext,
                                                                     L"Adaptive.Container",
                                                                     containerPanelAsFrameWorkElement.Get()));

        ComPtr<IAdaptiveActionElement> selectAction;
        RETURN_IF_FAILED(containerAsContainerBase->get_SelectAction(&selectAction));

        ComPtr<IUIElement> containerBorderAsUIElement;
        RETURN_IF_FAILED(containerBorder.As(&containerBorderAsUIElement));

        ComPtr<IAdaptiveHostConfig> hostConfig;
        RETURN_IF_FAILED(renderContext->get_HostConfig(&hostConfig));

        ActionHelpers::HandleSelectAction(adaptiveCardElement,
                                          selectAction.Get(),
                                          renderContext,
                                          containerBorderAsUIElement.Get(),
                                          XamlHelpers::SupportsInteractivity(hostConfig.Get()),
                                          true,
                                          containerControl);
        return S_OK;
    }
    CATCH_RETURN();


    rtxaml::UIElement AdaptiveContainerRenderer::Render(rtom::IAdaptiveCardElement cardElement,
                                                         rtrender::AdaptiveRenderContext renderContext,
                                                         rtrender::AdaptiveRenderArgs renderArgs)
    {
        try
        {
            /*ComPtr<IAdaptiveCardElement> cardElement(adaptiveCardElement);
            ComPtr<IAdaptiveContainer> adaptiveContainer;
            RETURN_IF_FAILED(cardElement.As(&adaptiveContainer));*/
            rtom::AdaptiveContainer adaptiveContainer = cardElement.as<rtom::AdaptiveContainer>();

           /* ComPtr<WholeItemsPanel> containerPanel;
            RETURN_IF_FAILED(MakeAndInitialize<WholeItemsPanel>(&containerPanel));*/
            rtrender::WholeItemsPanel containerPanel = winrt::make<rtrender::WholeItemsPanel>();

         /*   ComPtr<IFrameworkElement> containerPanelAsFrameWorkElement;
            RETURN_IF_FAILED(containerPanel.As(&containerPanelAsFrameWorkElement));*/

            // Get any RTL setting set on either the current context or on this container. Any value set on the
            // container should be set on the context to apply to all children
           /* renderContext.Rtl()
            ComPtr<IReference<bool>> previousContextRtl;
            RETURN_IF_FAILED(renderContext->get_Rtl(&previousContextRtl));*/
            //ComPtr<IReference<bool>> currentRtl = previousContextRtl;
            auto previousContextRtl = renderContext.Rtl();
            auto currentRtl = previousContextRtl;
            auto containerRtl = adaptiveContainer.Rtl();

            /*ComPtr<IReference<bool>> containerRtl;
            RETURN_IF_FAILED(adaptiveContainer->get_Rtl(&containerRtl));*/

            bool updatedRtl = false;
            if (containerRtl != nullptr)
            {
                currentRtl = containerRtl;
                //RETURN_IF_FAILED(renderContext->put_Rtl(currentRtl.Get()));
                renderContext.Rtl(currentRtl);
                updatedRtl = true;
            }

            if (currentRtl)
            {
                boolean rtlValue = currentRtl.GetBoolean();
                containerPanel.FlowDirection(rtlValue ? rtxaml::FlowDirection::RightToLeft : rtxaml::FlowDirection::LeftToRight);
                /*RETURN_IF_FAILED(currentRtl->get_Value(&rtlValue));*/

               /* RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_FlowDirection(rtlValue ? FlowDirection_RightToLeft :
                                                                                                FlowDirection_LeftToRight));*/
            }

            // Assign vertical alignment to strech so column will stretch and respect vertical content alignment
            /*ABI::AdaptiveCards::ObjectModel::WinUI3::HeightType containerHeightType{};
            RETURN_IF_FAILED(cardElement->get_Height(&containerHeightType));*/

            if (cardElement.Height() == rtom::HeightType::Auto)
            {
                //RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_VerticalAlignment(ABI::Windows::UI::Xaml::VerticalAlignment_Stretch));
                containerPanel.VerticalAlignment(rtxaml::VerticalAlignment::Stretch);
            }


           /* ComPtr<IAdaptiveContainerBase> containerAsContainerBase;
            RETURN_IF_FAILED(adaptiveContainer.As(&containerAsContainerBase));*/

            //UINT32 containerMinHeight{};
            //RETURN_IF_FAILED(containerAsContainerBase->get_MinHeight(&containerMinHeight));

            uint32_t containerMinHeight = adaptiveContainer.MinHeight();


            if (containerMinHeight > 0)
            {
                /*RETURN_IF_FAILED(containerPanelAsFrameWorkElement->put_MinHeight(containerMinHeight));*/
                containerPanel.MinHeight(containerMinHeight);
            }

            ComPtr<IBorder> containerBorder =
                XamlHelpers::CreateABIClass<IBorder>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Border));

            // TODO: Convert to FULL WINRT
            ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle;
            // winrt::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle;
            XamlHelpers::HandleStylingAndPadding(to_wrl(adaptiveContainer.as<rtom::IAdaptiveContainerBase>()).Get(),
                                                 containerBorder.Get(),
                                                 to_wrl(renderContext).Get(),
                                                 to_wrl(renderArgs).Get(),
                                                 &containerStyle);

           /* ComPtr<IFrameworkElement> parentElement;
            RETURN_IF_FAILED(renderArgs->get_ParentElement(&parentElement));*/
            auto parentElement = renderArgs.ParentElement();
            auto newRenderArgs =
                winrt::make<rtrender::implementation::AdaptiveRenderArgs>(containerStyle, parentElement, renderArgs);

          /*  ComPtr<IPanel> containerPanelAsPanel;
            RETURN_IF_FAILED(containerPanel.As(&containerPanelAsPanel));
            ComPtr<IVector<IAdaptiveCardElement*>> childItems;
            RETURN_IF_FAILED(adaptiveContainer->get_Items(&childItems));*/
            /*RETURN_IF_FAILED(XamlBuilder::BuildPanelChildren(
                childItems.Get(), containerPanelAsPanel.Get(), renderContext, newRenderArgs.Get(), [](IUIElement*) {}));*/
            /*RETURN_IF_FAILED(XamlBuilder::BuildPanelChildren(
                childItems.Get(), containerPanelAsPanel.Get(), renderContext, renderArgs, [](IUIElement*) {}));*/
            auto childItems = adaptiveContainer.Items();
            XamlBuilder::BuildPanelChildren(childItems,
                                            containerPanel.as<rtxaml::Controls::Panel>(),
                                            renderContext,
                                            renderArgs,
                                            [](rtxaml::UIElement) {});

            // If we changed the context's rtl setting, set it back after rendering the children
            if (updatedRtl)
            {
                //RETURN_IF_FAILED(renderContext->put_Rtl(previousContextRtl.Get()));
                renderContext.Rtl(previousContextRtl);
            }

           //ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::VerticalContentAlignment>> verticalContentAlignmentReference;
            //to_wrl(adaptiveContainer)->get_VerticalContentAlignment(&verticalContentAlignmentReference);
           auto verticalContentAlignmentReference = adaptiveContainer.VerticalContentAlignment();
            rtom::VerticalContentAlignment verticalContentAlignment =
                rtom::VerticalContentAlignment::Top;

            if (verticalContentAlignmentReference != nullptr)
            {
                verticalContentAlignment = verticalContentAlignmentReference.Value();
            }

            XamlHelpers::SetVerticalContentAlignmentToChildren(containerPanel,
                                                               verticalContentAlignment);

            // Check if backgroundImage defined
           /* ComPtr<IAdaptiveBackgroundImage> backgroundImage;
            BOOL backgroundImageIsValid;
            RETURN_IF_FAILED(adaptiveContainer->get_BackgroundImage(&backgroundImage));
            RETURN_IF_FAILED(IsBackgroundImageValid(backgroundImage.Get(), &backgroundImageIsValid));*/
            auto backgroundImage = adaptiveContainer.BackgroundImage();

            if (IsBackgroundImageValid(backgroundImage))
            {
                ComPtr<IGrid> rootElement =
                    XamlHelpers::CreateABIClass<IGrid>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Grid));
                ComPtr<IPanel> rootAsPanel;
                rootElement.As(&rootAsPanel);

                // Convert to WINRT
                /*XamlHelpers::ApplyBackgroundToRoot(rootAsPanel.Get(), backgroundImage.Get(), renderContext, newRenderArgs.Get());*/
                XamlHelpers::ApplyBackgroundToRoot(rootAsPanel.Get(), to_wrl(backgroundImage).Get(), to_wrl(renderContext).Get(), to_wrl(renderArgs).Get());

                // Add rootElement to containerBorder
                ComPtr<IUIElement> rootAsUIElement;
                rootElement.As(&rootAsUIElement);
                containerBorder->put_Child(rootAsUIElement.Get())

                // Add containerPanel to rootElement
                ComPtr<IFrameworkElement> containerPanelAsFElement;
                RETURN_IF_FAILED(containerPanel.As(&containerPanelAsFElement));
                XamlHelpers::AppendXamlElementToPanel(containerPanelAsFElement.Get(), rootAsPanel.Get(), containerHeightType);
            }
            else
            {
                // instead, directly add containerPanel to containerBorder
                ComPtr<IUIElement> containerPanelAsUIElement;
                RETURN_IF_FAILED(containerPanel.As(&containerPanelAsUIElement));
                RETURN_IF_FAILED(containerBorder->put_Child(containerPanelAsUIElement.Get()));
            }

            RETURN_IF_FAILED(XamlHelpers::SetStyleFromResourceDictionary(renderContext,
                                                                         L"Adaptive.Container",
                                                                         containerPanelAsFrameWorkElement.Get()));

            ComPtr<IAdaptiveActionElement> selectAction;
            RETURN_IF_FAILED(containerAsContainerBase->get_SelectAction(&selectAction));

            ComPtr<IUIElement> containerBorderAsUIElement;
            RETURN_IF_FAILED(containerBorder.As(&containerBorderAsUIElement));

            ComPtr<IAdaptiveHostConfig> hostConfig;
            RETURN_IF_FAILED(renderContext->get_HostConfig(&hostConfig));

            ActionHelpers::HandleSelectAction(adaptiveCardElement,
                                              selectAction.Get(),
                                              renderContext,
                                              containerBorderAsUIElement.Get(),
                                              XamlHelpers::SupportsInteractivity(hostConfig.Get()),
                                              true,
                                              containerControl);
            return S_OK;

    }
}
