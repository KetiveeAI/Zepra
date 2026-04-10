// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "widgets/widget.h"
#include <string>
#include <vector>
#include <functional>

namespace NXRender {

// ==================================================================
// HTML Form control base
// ==================================================================

class FormControl : public Widget {
public:
    FormControl();

    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    void setDisabled(bool d) { disabled_ = d; }
    bool disabled() const { return disabled_; }

    void setRequired(bool r) { required_ = r; }
    bool required() const { return required_; }

    void setReadOnly(bool ro) { readOnly_ = ro; }
    bool readOnly() const { return readOnly_; }

    void setAutofocus(bool af) { autofocus_ = af; }
    bool autofocus() const { return autofocus_; }

    virtual bool checkValidity() const { return true; }
    virtual std::string validationMessage() const { return ""; }
    void setCustomValidity(const std::string& msg) { customValidity_ = msg; }

    void setFormId(const std::string& id) { formId_ = id; }
    const std::string& formId() const { return formId_; }

    using ChangeCallback = std::function<void(FormControl*)>;
    void onChange(ChangeCallback cb) { onChange_ = cb; }

protected:
    std::string name_;
    bool disabled_ = false;
    bool required_ = false;
    bool readOnly_ = false;
    bool autofocus_ = false;
    std::string customValidity_;
    std::string formId_;
    ChangeCallback onChange_;

    void notifyChange();
};

// ==================================================================
// Input types
// ==================================================================

enum class InputType : uint8_t {
    Text, Password, Email, URL, Tel, Number, Search,
    Date, Time, DatetimeLocal, Month, Week,
    Color, Range, File,
    Hidden, Submit, Reset, Button, Image,
    Radio, Checkbox,
};

class InputWidget : public FormControl {
public:
    InputWidget(InputType type = InputType::Text);

    InputType inputType() const { return type_; }
    void setInputType(InputType t) { type_ = t; }

    void setValue(const std::string& val);
    const std::string& value() const { return value_; }

    void setPlaceholder(const std::string& p) { placeholder_ = p; }
    const std::string& placeholder() const { return placeholder_; }
    void setMaxLength(int max) { maxLength_ = max; }
    void setMinLength(int min) { minLength_ = min; }
    void setPattern(const std::string& pattern) { pattern_ = pattern; }
    void setAutocomplete(const std::string& ac) { autocomplete_ = ac; }

    void setMin(double min) { min_ = min; }
    void setMax(double max) { max_ = max; }
    void setStep(double step) { step_ = step; }
    double numericValue() const;

    void setChecked(bool c);
    bool checked() const { return checked_; }

    void setMultiple(bool m) { multiple_ = m; }
    bool multiple() const { return multiple_; }

    const std::vector<std::string>& files() const { return files_; }
    void setAccept(const std::string& accept) { accept_ = accept; }

    void setSelectionRange(int start, int end);
    int selectionStart() const { return selectionStart_; }
    int selectionEnd() const { return selectionEnd_; }

    bool checkValidity() const override;

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onKeyDown(KeyCode key, Modifiers mods) override;
    EventResult onTextInput(const std::string& text) override;

private:
    InputType type_;
    std::string value_;
    std::string placeholder_;
    int maxLength_ = -1;
    int minLength_ = -1;
    std::string pattern_;
    std::string autocomplete_;
    double min_ = 0, max_ = 100, step_ = 1;
    bool checked_ = false;
    bool multiple_ = false;
    std::vector<std::string> files_;
    std::string accept_;
    int selectionStart_ = 0, selectionEnd_ = 0;
    int cursorPos_ = 0;
    bool cursorVisible_ = true;
    float scrollOffset_ = 0;
};

// ==================================================================
// Textarea
// ==================================================================

class TextareaWidget : public FormControl {
public:
    TextareaWidget();

    void setValue(const std::string& val) { value_ = val; notifyChange(); }
    const std::string& value() const { return value_; }

    void setRows(int rows) { rows_ = rows; }
    void setCols(int cols) { cols_ = cols; }
    int rows() const { return rows_; }
    int cols() const { return cols_; }

    void setPlaceholder(const std::string& p) { placeholder_ = p; }
    void setWrap(const std::string& wrap) { wrap_ = wrap; }
    void setMaxLength(int max) { maxLength_ = max; }

    int selectionStart() const { return selectionStart_; }
    int selectionEnd() const { return selectionEnd_; }
    void setSelectionRange(int start, int end);

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;
    EventResult onKeyDown(KeyCode key, Modifiers mods) override;
    EventResult onTextInput(const std::string& text) override;
    EventResult onMouseDown(float x, float y, MouseButton button) override;

private:
    std::string value_;
    std::string placeholder_;
    int rows_ = 2, cols_ = 20;
    int maxLength_ = -1;
    std::string wrap_ = "soft";
    int selectionStart_ = 0, selectionEnd_ = 0;
    int scrollY_ = 0;
    int cursorLine_ = 0, cursorCol_ = 0;
};

// ==================================================================
// Select / Option
// ==================================================================

struct SelectOption {
    std::string value;
    std::string label;
    bool disabled = false;
    bool selected = false;
};

struct SelectOptGroup {
    std::string label;
    bool disabled = false;
    std::vector<SelectOption> options;
};

class SelectWidget : public FormControl {
public:
    SelectWidget();

    void addOption(const SelectOption& opt);
    void addOptGroup(const SelectOptGroup& group);
    void clearOptions();

    void setSelectedIndex(int index);
    int selectedIndex() const;
    std::string selectedValue() const;
    std::vector<int> selectedIndices() const;

    void setMultiple(bool m) { multiple_ = m; }
    bool multiple() const { return multiple_; }
    void setSize(int size) { visibleSize_ = size; }

    const std::vector<SelectOption>& flatOptions() const { return flatOptions_; }

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onKeyDown(KeyCode key, Modifiers mods) override;

private:
    std::vector<SelectOption> flatOptions_;
    std::vector<SelectOptGroup> optGroups_;
    bool multiple_ = false;
    int visibleSize_ = 1;
    bool dropdownOpen_ = false;
    int highlightedIndex_ = -1;
    float scrollOffset_ = 0;
};

// ==================================================================
// Progress bar
// ==================================================================

class ProgressWidget : public Widget {
public:
    ProgressWidget();

    void setValue(float val) { value_ = val; }
    float value() const { return value_; }
    void setMax(float max) { max_ = max; }
    float max() const { return max_; }
    bool indeterminate() const { return value_ < 0; }

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;

private:
    float value_ = -1;
    float max_ = 1.0f;
    float animOffset_ = 0;
};

// ==================================================================
// Meter
// ==================================================================

class MeterWidget : public Widget {
public:
    MeterWidget();

    void setValue(float val) { value_ = val; }
    float value() const { return value_; }
    void setMin(float min) { min_ = min; }
    void setMax(float max) { max_ = max; }
    void setLow(float low) { low_ = low; }
    void setHigh(float high) { high_ = high; }
    void setOptimum(float opt) { optimum_ = opt; }

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;

private:
    float value_ = 0, min_ = 0, max_ = 1;
    float low_ = 0, high_ = 1, optimum_ = 0.5f;
};

// ==================================================================
// Range slider
// ==================================================================

class RangeSliderWidget : public Widget {
public:
    RangeSliderWidget();

    void setValue(double val);
    double value() const { return value_; }
    void setMin(double min) { min_ = min; }
    void setMax(double max) { max_ = max; }
    void setStep(double step) { step_ = step; }
    void setOrientation(const std::string& orient) { vertical_ = (orient == "vertical"); }

    using ValueChangeCallback = std::function<void(double)>;
    void onValueChange(ValueChangeCallback cb) { onValueChange_ = cb; }

    Size measure(const Size& available) override;
    void render(GpuContext* ctx) override;
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onMouseMove(float x, float y) override;
    EventResult onMouseUp(float x, float y, MouseButton button) override;

private:
    double value_ = 50, min_ = 0, max_ = 100, step_ = 1;
    bool vertical_ = false;
    bool dragging_ = false;
    ValueChangeCallback onValueChange_;

    double snapToStep(double val) const;
    float valueToPosition(double val) const;
    double positionToValue(float pos) const;
};

} // namespace NXRender
