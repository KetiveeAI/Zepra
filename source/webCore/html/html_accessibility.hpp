/**
 * @file html_accessibility.hpp
 * @brief Accessibility (ARIA) support
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <unordered_map>

namespace Zepra::WebCore {

/**
 * @brief ARIA role categories
 */
enum class ARIARoleCategory {
    Abstract,
    Widget,
    Document,
    Landmark,
    LiveRegion,
    Window
};

/**
 * @brief Common ARIA roles
 */
enum class ARIARole {
    // Abstract (not directly usable)
    Command,
    Composite,
    Input,
    Landmark,
    Range,
    Roletype,
    Section,
    Sectionhead,
    Select,
    Structure,
    Widget,
    Window,
    
    // Widget roles
    Alert,
    AlertDialog,
    Button,
    Checkbox,
    Dialog,
    Gridcell,
    Link,
    Log,
    Marquee,
    Menuitem,
    Menuitemcheckbox,
    Menuitemradio,
    Option,
    Progressbar,
    Radio,
    Scrollbar,
    Searchbox,
    Slider,
    Spinbutton,
    Status,
    Switch,
    Tab,
    Tabpanel,
    Textbox,
    Timer,
    Tooltip,
    Treeitem,
    
    // Document structure
    Application,
    Article,
    Blockquote,
    Caption,
    Cell,
    Columnheader,
    Definition,
    Deletion,
    Directory,
    Document,
    Emphasis,
    Feed,
    Figure,
    Generic,
    Group,
    Heading,
    Img,
    Insertion,
    List,
    Listbox,
    Listitem,
    Math,
    Menu,
    Menubar,
    Meter,
    Navigation,
    None,
    Note,
    Paragraph,
    Presentation,
    Row,
    Rowgroup,
    Rowheader,
    Separator,
    Strong,
    Subscript,
    Superscript,
    Table,
    Tablist,
    Term,
    Time,
    Toolbar,
    Tree,
    Treegrid,
    
    // Landmark roles
    Banner,
    Complementary,
    Contentinfo,
    Form,
    Main,
    Region,
    Search,
    
    // Live regions
    // (Alert, Log, Marquee, Status, Timer already listed)
    
    // Window roles
    // (AlertDialog, Dialog already listed)
};

/**
 * @brief ARIA mixin for all elements
 */
class ARIAMixin {
public:
    virtual ~ARIAMixin() = default;
    
    // Role
    std::string role() const { return getAttribute("role"); }
    void setRole(const std::string& r) { setAttribute("role", r); }
    
    // Global ARIA attributes
    std::string ariaAtomic() const { return getAttribute("aria-atomic"); }
    void setAriaAtomic(const std::string& v) { setAttribute("aria-atomic", v); }
    
    std::string ariaBusy() const { return getAttribute("aria-busy"); }
    void setAriaBusy(const std::string& v) { setAttribute("aria-busy", v); }
    
    std::string ariaControls() const { return getAttribute("aria-controls"); }
    void setAriaControls(const std::string& v) { setAttribute("aria-controls", v); }
    
    std::string ariaCurrent() const { return getAttribute("aria-current"); }
    void setAriaCurrent(const std::string& v) { setAttribute("aria-current", v); }
    
    std::string ariaDescribedBy() const { return getAttribute("aria-describedby"); }
    void setAriaDescribedBy(const std::string& v) { setAttribute("aria-describedby", v); }
    
    std::string ariaDescription() const { return getAttribute("aria-description"); }
    void setAriaDescription(const std::string& v) { setAttribute("aria-description", v); }
    
    std::string ariaDetails() const { return getAttribute("aria-details"); }
    void setAriaDetails(const std::string& v) { setAttribute("aria-details", v); }
    
    std::string ariaDisabled() const { return getAttribute("aria-disabled"); }
    void setAriaDisabled(const std::string& v) { setAttribute("aria-disabled", v); }
    
    std::string ariaExpanded() const { return getAttribute("aria-expanded"); }
    void setAriaExpanded(const std::string& v) { setAttribute("aria-expanded", v); }
    
    std::string ariaHasPopup() const { return getAttribute("aria-haspopup"); }
    void setAriaHasPopup(const std::string& v) { setAttribute("aria-haspopup", v); }
    
    std::string ariaHidden() const { return getAttribute("aria-hidden"); }
    void setAriaHidden(const std::string& v) { setAttribute("aria-hidden", v); }
    
    std::string ariaLabel() const { return getAttribute("aria-label"); }
    void setAriaLabel(const std::string& v) { setAttribute("aria-label", v); }
    
    std::string ariaLabelledBy() const { return getAttribute("aria-labelledby"); }
    void setAriaLabelledBy(const std::string& v) { setAttribute("aria-labelledby", v); }
    
    std::string ariaLive() const { return getAttribute("aria-live"); }
    void setAriaLive(const std::string& v) { setAttribute("aria-live", v); }
    
    std::string ariaOwns() const { return getAttribute("aria-owns"); }
    void setAriaOwns(const std::string& v) { setAttribute("aria-owns", v); }
    
    std::string ariaRelevant() const { return getAttribute("aria-relevant"); }
    void setAriaRelevant(const std::string& v) { setAttribute("aria-relevant", v); }
    
    std::string ariaRoleDescription() const { return getAttribute("aria-roledescription"); }
    void setAriaRoleDescription(const std::string& v) { setAttribute("aria-roledescription", v); }
    
protected:
    virtual std::string getAttribute(const std::string& name) const = 0;
    virtual void setAttribute(const std::string& name, const std::string& value) = 0;
};

/**
 * @brief Accessible name computation
 */
class AccessibleName {
public:
    static std::string computeAccessibleName(HTMLElement* element);
    static std::string computeAccessibleDescription(HTMLElement* element);
};

} // namespace Zepra::WebCore
