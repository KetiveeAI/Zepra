#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace zepra {

// Forward declarations
class DOMNode;
class CSSRule;
class RenderObject;
class Tab;
class Window;

// Basic types
using String = std::string;
using StringView = std::string_view;
using Size = size_t;
using Index = int;

// DOM Types
enum class NodeType {
    ELEMENT,
    TEXT,
    COMMENT,
    DOCUMENT,
    DOCUMENT_TYPE
};

enum class ElementType {
    HTML,
    HEAD,
    BODY,
    DIV,
    SPAN,
    P,
    H1, H2, H3, H4, H5, H6,
    A,
    IMG,
    INPUT,
    BUTTON,
    FORM,
    TABLE,
    TR,
    TD,
    TH,
    UL,
    OL,
    LI,
    SCRIPT,
    STYLE,
    LINK,
    META,
    TITLE,
    UNKNOWN
};

// CSS Types
enum class CSSPropertyType {
    COLOR,
    BACKGROUND_COLOR,
    FONT_SIZE,
    FONT_FAMILY,
    FONT_WEIGHT,
    WIDTH,
    HEIGHT,
    MARGIN,
    PADDING,
    BORDER,
    DISPLAY,
    POSITION,
    TOP,
    LEFT,
    RIGHT,
    BOTTOM,
    Z_INDEX,
    OPACITY,
    VISIBILITY,
    UNKNOWN
};

enum class CSSValueType {
    KEYWORD,
    LENGTH,
    PERCENTAGE,
    COLOR,
    STRING,
    URL,
    FUNCTION,
    UNKNOWN
};

enum class DisplayType {
    BLOCK,
    INLINE,
    INLINE_BLOCK,
    FLEX,
    GRID,
    NONE
};

enum class PositionType {
    STATIC,
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
    FIXED,
    STICKY
};

// Layout Types
struct Rect {
    float x, y, width, height;
    
    constexpr Rect() : x(0), y(0), width(0), height(0) {}
    constexpr Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

struct Point {
    float x, y;
    
    constexpr Point() : x(0), y(0) {}
    constexpr Point(float x, float y) : x(x), y(y) {}
};

struct Color {
    uint8_t r, g, b, a;
    
    constexpr Color() : r(0), g(0), b(0), a(255) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    
    static constexpr Color Black() { return Color(0, 0, 0); }
    static constexpr Color White() { return Color(255, 255, 255); }
    static constexpr Color Red() { return Color(255, 0, 0); }
    static constexpr Color Green() { return Color(0, 255, 0); }
    static constexpr Color Blue() { return Color(0, 0, 255); }
    static constexpr Color Transparent() { return Color(0, 0, 0, 0); }
};

// Browser Types
enum class TabState {
    LOADING,
    COMPLETED,
    TAB_ERROR,
    CRASHED
};

enum class NavigationType {
    NORMAL,
    RELOAD,
    BACK,
    FORWARD
};

// Event Types
enum class EventType {
    CLICK,
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE,
    KEY_DOWN,
    KEY_UP,
    SCROLL,
    RESIZE,
    LOAD,
    UNLOAD
};

// Network Types
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    HTTP_DELETE,
    HEAD,
    OPTIONS
};

enum class HTTPStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502
};

// Search Types
enum class SearchEngineType {
    KETIVEE_SEARCH,
    GOOGLE,
    BING,
    DUCKDUCKGO,
    CUSTOM
};

// Callback Types
using EventCallback = std::function<void(const EventType&, const void*)>;
using NavigationCallback = std::function<void(const String&)>;
using SearchCallback = std::function<void(const String&, const std::vector<String>&)>;

// Smart Pointers
using DOMNodePtr = std::shared_ptr<DOMNode>;
using CSSRulePtr = std::shared_ptr<CSSRule>;
using RenderObjectPtr = std::shared_ptr<RenderObject>;
using TabPtr = std::shared_ptr<Tab>;
using WindowPtr = std::shared_ptr<Window>;

// Containers
using AttributeMap = std::unordered_map<String, String>;
using StyleMap = std::unordered_map<CSSPropertyType, String>;
using TabList = std::vector<TabPtr>;
using DOMNodeList = std::vector<DOMNodePtr>;

} // namespace zepra 