// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_element_factory.cpp
 * @brief Factory implementation for creating HTML elements by tag name
 */

#include "html/html_element_factory.hpp"

// Structural elements
#include "html/html_head_element.hpp"
#include "html/html_body_element.hpp"
#include "html/html_title_element.hpp"
#include "html/html_base_element.hpp"
#include "html/html_link_element.hpp"
#include "html/html_meta_element.hpp"
#include "html/html_style_element.hpp"

// Text elements
#include "html/html_paragraph_element.hpp"
#include "html/html_break_element.hpp"
#include "html/html_preformatted_element.hpp"
#include "html/html_quote_element.hpp"
#include "html/html_time_element.hpp"
#include "html/html_text_formatting.hpp"

// Form elements
#include "html/html_form_element.hpp"
#include "html/html_input_element.hpp"
#include "html/html_button_element.hpp"
#include "html/html_select_element.hpp"
#include "html/html_textarea_element.hpp"
#include "html/html_option_element.hpp"
#include "html/html_fieldset_element.hpp"
#include "html/html_progress_element.hpp"
#include "html/html_meter_element.hpp"
#include "html/html_datalist_element.hpp"
#include "html/html_output_element.hpp"

// Media elements
#include "html/html_image_element.hpp"
#include "html/html_audio_element.hpp"
#include "html/html_video_element.hpp"
#include "html/html_source_element.hpp"
#include "html/html_picture_element.hpp"
#include "html/html_canvas_element.hpp"

// Interactive elements
#include "html/html_anchor_element.hpp"
#include "html/html_details_element.hpp"
#include "html/html_template_element.hpp"
#include "html/html_slot_element.hpp"
#include "html/html_map_element.hpp"

// Container elements
#include "html/html_div_element.hpp"
#include "html/html_span_element.hpp"
#include "html/html_heading_element.hpp"
#include "html/html_list_element.hpp"
#include "html/html_table_element.hpp"
#include "html/html_script_element.hpp"
#include "html/html_iframe_element.hpp"

#include <algorithm>
#include <cctype>

namespace Zepra::WebCore {

HTMLElementFactory& HTMLElementFactory::instance() {
    static HTMLElementFactory factory;
    return factory;
}

HTMLElementFactory::HTMLElementFactory() {
    registerStandardElements();
}

std::unique_ptr<HTMLElement> HTMLElementFactory::createElement(const std::string& tagName) const {
    // Convert to lowercase for lookup
    std::string lowerTag = tagName;
    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    auto it = creators_.find(lowerTag);
    if (it != creators_.end()) {
        return it->second();
    }
    
    // Return generic HTMLElement for unknown tags
    return std::make_unique<HTMLElement>(lowerTag);
}

void HTMLElementFactory::registerElement(const std::string& tagName, Creator creator) {
    std::string lowerTag = tagName;
    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    creators_[lowerTag] = std::move(creator);
}

bool HTMLElementFactory::isKnownElement(const std::string& tagName) const {
    std::string lowerTag = tagName;
    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return creators_.find(lowerTag) != creators_.end();
}

std::vector<std::string> HTMLElementFactory::registeredElements() const {
    std::vector<std::string> tags;
    tags.reserve(creators_.size());
    for (const auto& [tag, _] : creators_) {
        tags.push_back(tag);
    }
    return tags;
}

void HTMLElementFactory::registerStandardElements() {
    // =========================================================================
    // Document Structure
    // =========================================================================
    registerElement("html", []() { return std::make_unique<HTMLElement>("html"); });
    registerElement("head", []() { return std::make_unique<HTMLHeadElement>(); });
    registerElement("body", []() { return std::make_unique<HTMLBodyElement>(); });
    registerElement("title", []() { return std::make_unique<HTMLTitleElement>(); });
    registerElement("base", []() { return std::make_unique<HTMLBaseElement>(); });
    registerElement("link", []() { return std::make_unique<HTMLLinkElement>(); });
    registerElement("meta", []() { return std::make_unique<HTMLMetaElement>(); });
    registerElement("style", []() { return std::make_unique<HTMLStyleElement>(); });
    
    // =========================================================================
    // Scripting
    // =========================================================================
    registerElement("script", []() { return std::make_unique<HTMLScriptElement>(); });
    registerElement("noscript", []() { return std::make_unique<HTMLElement>("noscript"); });
    registerElement("template", []() { return std::make_unique<HTMLTemplateElement>(); });
    registerElement("slot", []() { return std::make_unique<HTMLSlotElement>(); });
    
    // =========================================================================
    // Content Sectioning
    // =========================================================================
    registerElement("article", []() { return std::make_unique<HTMLElement>("article"); });
    registerElement("aside", []() { return std::make_unique<HTMLElement>("aside"); });
    registerElement("footer", []() { return std::make_unique<HTMLElement>("footer"); });
    registerElement("header", []() { return std::make_unique<HTMLElement>("header"); });
    registerElement("main", []() { return std::make_unique<HTMLElement>("main"); });
    registerElement("nav", []() { return std::make_unique<HTMLElement>("nav"); });
    registerElement("section", []() { return std::make_unique<HTMLElement>("section"); });
    registerElement("address", []() { return std::make_unique<HTMLAddressElement>(); });
    
    // =========================================================================
    // Headings
    // =========================================================================
    registerElement("h1", []() { return std::make_unique<HTMLHeadingElement>(1); });
    registerElement("h2", []() { return std::make_unique<HTMLHeadingElement>(2); });
    registerElement("h3", []() { return std::make_unique<HTMLHeadingElement>(3); });
    registerElement("h4", []() { return std::make_unique<HTMLHeadingElement>(4); });
    registerElement("h5", []() { return std::make_unique<HTMLHeadingElement>(5); });
    registerElement("h6", []() { return std::make_unique<HTMLHeadingElement>(6); });
    registerElement("hgroup", []() { return std::make_unique<HTMLElement>("hgroup"); });
    
    // =========================================================================
    // Text Content
    // =========================================================================
    registerElement("div", []() { return std::make_unique<HTMLDivElement>(); });
    registerElement("p", []() { return std::make_unique<HTMLParagraphElement>(); });
    registerElement("pre", []() { return std::make_unique<HTMLPreElement>(); });
    registerElement("blockquote", []() { return std::make_unique<HTMLBlockQuoteElement>(); });
    registerElement("hr", []() { return std::make_unique<HTMLHRElement>(); });
    registerElement("figure", []() { return std::make_unique<HTMLElement>("figure"); });
    registerElement("figcaption", []() { return std::make_unique<HTMLElement>("figcaption"); });
    registerElement("menu", []() { return std::make_unique<HTMLElement>("menu"); });
    
    // =========================================================================
    // Lists
    // =========================================================================
    registerElement("ul", []() { return std::make_unique<HTMLUListElement>(); });
    registerElement("ol", []() { return std::make_unique<HTMLOListElement>(); });
    registerElement("li", []() { return std::make_unique<HTMLLIElement>(); });
    registerElement("dl", []() { return std::make_unique<HTMLDListElement>(); });
    registerElement("dt", []() { return std::make_unique<HTMLElement>("dt"); });
    registerElement("dd", []() { return std::make_unique<HTMLElement>("dd"); });
    
    // =========================================================================
    // Inline Text Semantics
    // =========================================================================
    registerElement("span", []() { return std::make_unique<HTMLSpanElement>(); });
    registerElement("a", []() { return std::make_unique<HTMLAnchorElement>(); });
    registerElement("br", []() { return std::make_unique<HTMLBRElement>(); });
    registerElement("wbr", []() { return std::make_unique<HTMLWBRElement>(); });
    registerElement("em", []() { return std::make_unique<HTMLEmElement>(); });
    registerElement("strong", []() { return std::make_unique<HTMLStrongElement>(); });
    registerElement("small", []() { return std::make_unique<HTMLSmallElement>(); });
    registerElement("s", []() { return std::make_unique<HTMLSElement>(); });
    registerElement("cite", []() { return std::make_unique<HTMLCiteElement>(); });
    registerElement("q", []() { return std::make_unique<HTMLQElement>(); });
    registerElement("dfn", []() { return std::make_unique<HTMLDfnElement>(); });
    registerElement("abbr", []() { return std::make_unique<HTMLAbbrElement>(); });
    registerElement("code", []() { return std::make_unique<HTMLCodeElement>(); });
    registerElement("var", []() { return std::make_unique<HTMLVarElement>(); });
    registerElement("samp", []() { return std::make_unique<HTMLSampElement>(); });
    registerElement("kbd", []() { return std::make_unique<HTMLKbdElement>(); });
    registerElement("sub", []() { return std::make_unique<HTMLSubElement>(); });
    registerElement("sup", []() { return std::make_unique<HTMLSupElement>(); });
    registerElement("i", []() { return std::make_unique<HTMLIElement>(); });
    registerElement("b", []() { return std::make_unique<HTMLBElement>(); });
    registerElement("u", []() { return std::make_unique<HTMLUElement>(); });
    registerElement("mark", []() { return std::make_unique<HTMLMarkElement>(); });
    registerElement("time", []() { return std::make_unique<HTMLTimeElement>(); });
    registerElement("data", []() { return std::make_unique<HTMLElement>("data"); });
    registerElement("ruby", []() { return std::make_unique<HTMLElement>("ruby"); });
    registerElement("rt", []() { return std::make_unique<HTMLElement>("rt"); });
    registerElement("rp", []() { return std::make_unique<HTMLElement>("rp"); });
    registerElement("bdi", []() { return std::make_unique<HTMLElement>("bdi"); });
    registerElement("bdo", []() { return std::make_unique<HTMLElement>("bdo"); });
    
    // =========================================================================
    // Edits
    // =========================================================================
    registerElement("ins", []() { return std::make_unique<HTMLInsElement>(); });
    registerElement("del", []() { return std::make_unique<HTMLDelElement>(); });
    
    // =========================================================================
    // Embedded Content
    // =========================================================================
    registerElement("img", []() { return std::make_unique<HTMLImageElement>(); });
    registerElement("picture", []() { return std::make_unique<HTMLPictureElement>(); });
    registerElement("source", []() { return std::make_unique<HTMLSourceElement>(); });
    registerElement("iframe", []() { return std::make_unique<HTMLIFrameElement>(); });
    registerElement("embed", []() { return std::make_unique<HTMLElement>("embed"); });
    registerElement("object", []() { return std::make_unique<HTMLElement>("object"); });
    registerElement("param", []() { return std::make_unique<HTMLElement>("param"); });
    registerElement("video", []() { return std::make_unique<HTMLVideoElement>(); });
    registerElement("audio", []() { return std::make_unique<HTMLAudioElement>(); });
    registerElement("track", []() { return std::make_unique<HTMLElement>("track"); });
    registerElement("map", []() { return std::make_unique<HTMLMapElement>(); });
    registerElement("area", []() { return std::make_unique<HTMLAreaElement>(); });
    registerElement("canvas", []() { return std::make_unique<HTMLCanvasElement>(); });
    registerElement("svg", []() { return std::make_unique<HTMLElement>("svg"); });
    registerElement("math", []() { return std::make_unique<HTMLElement>("math"); });
    
    // =========================================================================
    // Table Content
    // =========================================================================
    registerElement("table", []() { return std::make_unique<HTMLTableElement>(); });
    registerElement("caption", []() { return std::make_unique<HTMLTableCaptionElement>(); });
    registerElement("thead", []() { return std::make_unique<HTMLTableSectionElement>(HTMLTableSectionElement::Type::THead); });
    registerElement("tbody", []() { return std::make_unique<HTMLTableSectionElement>(HTMLTableSectionElement::Type::TBody); });
    registerElement("tfoot", []() { return std::make_unique<HTMLTableSectionElement>(HTMLTableSectionElement::Type::TFoot); });
    registerElement("tr", []() { return std::make_unique<HTMLTableRowElement>(); });
    registerElement("th", []() { return std::make_unique<HTMLTableCellElement>(HTMLTableCellElement::Type::TH); });
    registerElement("td", []() { return std::make_unique<HTMLTableCellElement>(HTMLTableCellElement::Type::TD); });
    registerElement("col", []() { return std::make_unique<HTMLTableColElement>(HTMLTableColElement::Type::Col); });
    registerElement("colgroup", []() { return std::make_unique<HTMLTableColElement>(HTMLTableColElement::Type::ColGroup); });
    
    // =========================================================================
    // Forms
    // =========================================================================
    registerElement("form", []() { return std::make_unique<HTMLFormElement>(); });
    registerElement("input", []() { return std::make_unique<HTMLInputElement>(); });
    registerElement("button", []() { return std::make_unique<HTMLButtonElement>(); });
    registerElement("select", []() { return std::make_unique<HTMLSelectElement>(); });
    registerElement("datalist", []() { return std::make_unique<HTMLDataListElement>(); });
    registerElement("optgroup", []() { return std::make_unique<HTMLOptGroupElement>(); });
    registerElement("option", []() { return std::make_unique<HTMLOptionElement>(); });
    registerElement("textarea", []() { return std::make_unique<HTMLTextAreaElement>(); });
    registerElement("output", []() { return std::make_unique<HTMLOutputElement>(); });
    registerElement("progress", []() { return std::make_unique<HTMLProgressElement>(); });
    registerElement("meter", []() { return std::make_unique<HTMLMeterElement>(); });
    registerElement("fieldset", []() { return std::make_unique<HTMLFieldSetElement>(); });
    registerElement("legend", []() { return std::make_unique<HTMLLegendElement>(); });
    registerElement("label", []() { return std::make_unique<HTMLElement>("label"); });
    
    // =========================================================================
    // Interactive Elements
    // =========================================================================
    registerElement("details", []() { return std::make_unique<HTMLDetailsElement>(); });
    registerElement("summary", []() { return std::make_unique<HTMLSummaryElement>(); });
    registerElement("dialog", []() { return std::make_unique<HTMLElement>("dialog"); });
    
    // =========================================================================
    // Deprecated but still supported
    // =========================================================================
    registerElement("center", []() { return std::make_unique<HTMLElement>("center"); });
    registerElement("font", []() { return std::make_unique<HTMLElement>("font"); });
    registerElement("marquee", []() { return std::make_unique<HTMLElement>("marquee"); });
}

} // namespace Zepra::WebCore
