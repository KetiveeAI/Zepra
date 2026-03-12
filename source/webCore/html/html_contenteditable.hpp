/**
 * @file html_contenteditable.hpp
 * @brief Content editable and editing APIs
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Content editable mode
 */
enum class ContentEditableMode {
    False,
    True,
    PlainTextOnly,
    Inherit
};

/**
 * @brief Input event types
 */
enum class InputEventType {
    InsertText,
    InsertReplacementText,
    InsertLineBreak,
    InsertParagraph,
    InsertOrderedList,
    InsertUnorderedList,
    InsertLink,
    InsertHorizontalRule,
    DeleteContentBackward,
    DeleteContentForward,
    DeleteWordBackward,
    DeleteWordForward,
    DeleteSoftLineBackward,
    DeleteSoftLineForward,
    DeleteHardLineBackward,
    DeleteHardLineForward,
    HistoryUndo,
    HistoryRedo,
    FormatBold,
    FormatItalic,
    FormatUnderline,
    FormatStrikethrough,
    FormatSuperscript,
    FormatSubscript,
    FormatIndent,
    FormatOutdent,
    FormatRemove
};

/**
 * @brief Input event for contenteditable
 */
struct InputEvent {
    InputEventType inputType;
    std::string data;
    bool isComposing = false;
    class Range* targetRanges = nullptr;
};

/**
 * @brief Before input event
 */
struct BeforeInputEvent : InputEvent {
    bool defaultPrevented = false;
    void preventDefault() { defaultPrevented = true; }
};

/**
 * @brief Composition event
 */
struct CompositionEvent {
    std::string type;  // compositionstart, compositionupdate, compositionend
    std::string data;
};

/**
 * @brief ContentEditable mixin
 */
class ContentEditable {
public:
    virtual ~ContentEditable() = default;
    
    ContentEditableMode contentEditable() const { return contentEditable_; }
    void setContentEditable(ContentEditableMode mode);
    
    bool isContentEditable() const;
    
    // Input method support
    std::string inputMode() const { return inputMode_; }
    void setInputMode(const std::string& mode) { inputMode_ = mode; }
    
    std::string enterKeyHint() const { return enterKeyHint_; }
    void setEnterKeyHint(const std::string& hint) { enterKeyHint_ = hint; }
    
    // Spellcheck
    bool spellcheck() const { return spellcheck_; }
    void setSpellcheck(bool s) { spellcheck_ = s; }
    
    // Autocomplete
    bool autocapitalize() const { return autocapitalize_; }
    void setAutocapitalize(bool a) { autocapitalize_ = a; }
    
    std::string autocorrect() const { return autocorrect_; }
    void setAutocorrect(const std::string& a) { autocorrect_ = a; }
    
protected:
    ContentEditableMode contentEditable_ = ContentEditableMode::Inherit;
    std::string inputMode_ = "text";  // none, text, decimal, numeric, etc.
    std::string enterKeyHint_ = "enter";  // enter, done, go, next, previous, search, send
    bool spellcheck_ = true;
    bool autocapitalize_ = true;
    std::string autocorrect_ = "on";
};

/**
 * @brief Editable host interface
 */
class EditableHost {
public:
    virtual ~EditableHost() = default;
    
    virtual void execCommand(const std::string& command,
                             const std::string& value = "") = 0;
    virtual bool queryCommandEnabled(const std::string& command) const = 0;
    virtual bool queryCommandState(const std::string& command) const = 0;
    virtual std::string queryCommandValue(const std::string& command) const = 0;
};

} // namespace Zepra::WebCore
