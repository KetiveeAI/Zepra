// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "css_math.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <unordered_map>

namespace NXRender {
namespace Web {

// ==================================================================
// CSSUnitValue
// ==================================================================

float CSSUnitValue::toPx(float fontSize, float rootFontSize, float viewWidth,
                          float viewHeight, float parentSize) const {
    switch (unit) {
        case CSSUnit::Px: case CSSUnit::Number: return value;
        case CSSUnit::Em: return value * fontSize;
        case CSSUnit::Rem: return value * rootFontSize;
        case CSSUnit::Percent: return value * parentSize * 0.01f;
        case CSSUnit::Vw: case CSSUnit::Svw: case CSSUnit::Lvw: case CSSUnit::Dvw:
            return value * viewWidth * 0.01f;
        case CSSUnit::Vh: case CSSUnit::Svh: case CSSUnit::Lvh: case CSSUnit::Dvh:
            return value * viewHeight * 0.01f;
        case CSSUnit::Vmin: return value * std::min(viewWidth, viewHeight) * 0.01f;
        case CSSUnit::Vmax: return value * std::max(viewWidth, viewHeight) * 0.01f;
        case CSSUnit::Ch: return value * fontSize * 0.5f;
        case CSSUnit::Ex: return value * fontSize * 0.5f;
        case CSSUnit::Ic: return value * fontSize;
        case CSSUnit::Lh: case CSSUnit::Rlh: return value * fontSize * 1.2f;
        case CSSUnit::Cm: return value * 96.0f / 2.54f;
        case CSSUnit::Mm: return value * 96.0f / 25.4f;
        case CSSUnit::In: return value * 96.0f;
        case CSSUnit::Pt: return value * 96.0f / 72.0f;
        case CSSUnit::Pc: return value * 96.0f / 6.0f;
        case CSSUnit::Q: return value * 96.0f / (2.54f * 4.0f);
        case CSSUnit::Fr: return value;
        case CSSUnit::Deg: return value;
        case CSSUnit::Rad: return value * 180.0f / 3.14159265f;
        case CSSUnit::Grad: return value * 0.9f;
        case CSSUnit::Turn: return value * 360.0f;
        case CSSUnit::S: return value * 1000.0f;
        case CSSUnit::Ms: return value;
        case CSSUnit::Dpi: return value;
        case CSSUnit::Dpcm: return value * 2.54f;
        case CSSUnit::Dppx: return value * 96.0f;
        case CSSUnit::None: return value;
    }
    return value;
}

CSSUnit CSSUnitValue::parseUnit(const std::string& str) {
    static const std::unordered_map<std::string, CSSUnit> map = {
        {"px", CSSUnit::Px}, {"em", CSSUnit::Em}, {"rem", CSSUnit::Rem},
        {"%", CSSUnit::Percent}, {"vw", CSSUnit::Vw}, {"vh", CSSUnit::Vh},
        {"vmin", CSSUnit::Vmin}, {"vmax", CSSUnit::Vmax},
        {"ch", CSSUnit::Ch}, {"ex", CSSUnit::Ex}, {"ic", CSSUnit::Ic},
        {"lh", CSSUnit::Lh}, {"rlh", CSSUnit::Rlh},
        {"cm", CSSUnit::Cm}, {"mm", CSSUnit::Mm}, {"in", CSSUnit::In},
        {"pt", CSSUnit::Pt}, {"pc", CSSUnit::Pc}, {"q", CSSUnit::Q},
        {"svw", CSSUnit::Svw}, {"svh", CSSUnit::Svh},
        {"lvw", CSSUnit::Lvw}, {"lvh", CSSUnit::Lvh},
        {"dvw", CSSUnit::Dvw}, {"dvh", CSSUnit::Dvh},
        {"fr", CSSUnit::Fr},
        {"deg", CSSUnit::Deg}, {"rad", CSSUnit::Rad},
        {"grad", CSSUnit::Grad}, {"turn", CSSUnit::Turn},
        {"s", CSSUnit::S}, {"ms", CSSUnit::Ms},
        {"dpi", CSSUnit::Dpi}, {"dpcm", CSSUnit::Dpcm}, {"dppx", CSSUnit::Dppx},
    };
    auto it = map.find(str);
    return (it != map.end()) ? it->second : CSSUnit::None;
}

CSSUnitValue CSSUnitValue::parse(const std::string& str) {
    CSSUnitValue result;
    char* end;
    result.value = std::strtof(str.c_str(), &end);
    if (end && *end) {
        result.unit = parseUnit(std::string(end));
    } else {
        result.unit = CSSUnit::Number;
    }
    return result;
}

bool CSSUnitValue::isAbsolute(CSSUnit u) {
    return u == CSSUnit::Px || u == CSSUnit::Cm || u == CSSUnit::Mm ||
           u == CSSUnit::In || u == CSSUnit::Pt || u == CSSUnit::Pc || u == CSSUnit::Q;
}

bool CSSUnitValue::isRelative(CSSUnit u) {
    return u == CSSUnit::Em || u == CSSUnit::Rem || u == CSSUnit::Percent ||
           u == CSSUnit::Vw || u == CSSUnit::Vh || u == CSSUnit::Vmin || u == CSSUnit::Vmax ||
           u == CSSUnit::Ch || u == CSSUnit::Ex || u == CSSUnit::Fr;
}

bool CSSUnitValue::isAngle(CSSUnit u) {
    return u == CSSUnit::Deg || u == CSSUnit::Rad || u == CSSUnit::Grad || u == CSSUnit::Turn;
}

bool CSSUnitValue::isTime(CSSUnit u) {
    return u == CSSUnit::S || u == CSSUnit::Ms;
}

// ==================================================================
// CSSMathExpr
// ==================================================================

std::unique_ptr<CSSMathExpr> CSSMathExpr::makeValue(float v, CSSUnit u) {
    auto e = std::make_unique<CSSMathExpr>();
    e->op = Op::Value;
    e->value = {v, u};
    return e;
}

std::unique_ptr<CSSMathExpr> CSSMathExpr::makeBinary(Op op,
                                                       std::unique_ptr<CSSMathExpr> lhs,
                                                       std::unique_ptr<CSSMathExpr> rhs) {
    auto e = std::make_unique<CSSMathExpr>();
    e->op = op;
    e->children.push_back(std::move(lhs));
    e->children.push_back(std::move(rhs));
    return e;
}

std::unique_ptr<CSSMathExpr> CSSMathExpr::makeFunction(Op op,
                                                         std::vector<std::unique_ptr<CSSMathExpr>> args) {
    auto e = std::make_unique<CSSMathExpr>();
    e->op = op;
    e->children = std::move(args);
    return e;
}

CSSMathExpr CSSMathExpr::clone() const {
    CSSMathExpr copy;
    copy.op = op;
    copy.value = value;
    for (const auto& child : children) {
        auto c = std::make_unique<CSSMathExpr>(child->clone());
        copy.children.push_back(std::move(c));
    }
    return copy;
}

// ==================================================================
// CSSCalcEngine — Parser
// ==================================================================

bool CSSCalcEngine::isMathFunction(const std::string& value) {
    return value.find("calc(") == 0 || value.find("min(") == 0 ||
           value.find("max(") == 0 || value.find("clamp(") == 0 ||
           value.find("abs(") == 0 || value.find("sign(") == 0 ||
           value.find("round(") == 0 || value.find("mod(") == 0 ||
           value.find("sin(") == 0 || value.find("cos(") == 0 ||
           value.find("tan(") == 0 || value.find("sqrt(") == 0 ||
           value.find("pow(") == 0 || value.find("hypot(") == 0;
}

void CSSCalcEngine::Parser::skipWhitespace() {
    while (pos < input.size() && std::isspace(input[pos])) pos++;
}

bool CSSCalcEngine::Parser::match(char c) {
    skipWhitespace();
    if (pos < input.size() && input[pos] == c) { pos++; return true; }
    return false;
}

bool CSSCalcEngine::Parser::peek(char c) const {
    size_t p = pos;
    while (p < input.size() && std::isspace(input[p])) p++;
    return p < input.size() && input[p] == c;
}

std::string CSSCalcEngine::Parser::readNumber() {
    size_t start = pos;
    if (pos < input.size() && (input[pos] == '-' || input[pos] == '+')) pos++;
    while (pos < input.size() && (std::isdigit(input[pos]) || input[pos] == '.')) pos++;
    // Scientific notation
    if (pos < input.size() && (input[pos] == 'e' || input[pos] == 'E')) {
        pos++;
        if (pos < input.size() && (input[pos] == '-' || input[pos] == '+')) pos++;
        while (pos < input.size() && std::isdigit(input[pos])) pos++;
    }
    return input.substr(start, pos - start);
}

std::string CSSCalcEngine::Parser::readUnit() {
    size_t start = pos;
    while (pos < input.size() && (std::isalpha(input[pos]) || input[pos] == '%')) pos++;
    return input.substr(start, pos - start);
}

std::string CSSCalcEngine::Parser::readIdentifier() {
    size_t start = pos;
    while (pos < input.size() && (std::isalpha(input[pos]) || input[pos] == '-')) pos++;
    return input.substr(start, pos - start);
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::Parser::parseExpression() {
    auto left = parseTerm();
    while (true) {
        skipWhitespace();
        if (pos >= input.size()) break;
        if (input[pos] == '+' && pos + 1 < input.size() && std::isspace(input[pos + 1])) {
            pos++;
            auto right = parseTerm();
            left = CSSMathExpr::makeBinary(CSSMathExpr::Op::Add, std::move(left), std::move(right));
        } else if (input[pos] == '-' && pos + 1 < input.size() && std::isspace(input[pos + 1])) {
            pos++;
            auto right = parseTerm();
            left = CSSMathExpr::makeBinary(CSSMathExpr::Op::Subtract, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::Parser::parseTerm() {
    auto left = parseFactor();
    while (true) {
        skipWhitespace();
        if (pos >= input.size()) break;
        if (input[pos] == '*') {
            pos++;
            auto right = parseFactor();
            left = CSSMathExpr::makeBinary(CSSMathExpr::Op::Multiply, std::move(left), std::move(right));
        } else if (input[pos] == '/') {
            pos++;
            auto right = parseFactor();
            left = CSSMathExpr::makeBinary(CSSMathExpr::Op::Divide, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::Parser::parseFactor() {
    return parseAtom();
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::Parser::parseAtom() {
    skipWhitespace();
    if (pos >= input.size()) return CSSMathExpr::makeValue(0);

    // Parenthesized expression
    if (input[pos] == '(') {
        pos++;
        auto expr = parseExpression();
        match(')');
        return expr;
    }

    // Negative sign
    if (input[pos] == '-' && pos + 1 < input.size() && !std::isspace(input[pos + 1])) {
        pos++;
        auto inner = parseAtom();
        std::vector<std::unique_ptr<CSSMathExpr>> args;
        args.push_back(std::move(inner));
        return CSSMathExpr::makeFunction(CSSMathExpr::Op::Negate, std::move(args));
    }

    // Number with optional unit
    if (std::isdigit(input[pos]) || input[pos] == '.' ||
        (input[pos] == '+' && pos + 1 < input.size() && std::isdigit(input[pos + 1]))) {
        std::string num = readNumber();
        std::string unit = readUnit();
        float v = std::strtof(num.c_str(), nullptr);
        CSSUnit u = unit.empty() ? CSSUnit::Number : CSSUnitValue::parseUnit(unit);
        return CSSMathExpr::makeValue(v, u);
    }

    // Function call
    if (std::isalpha(input[pos])) {
        std::string name = readIdentifier();
        if (match('(')) {
            return parseFunction(name);
        }
        // Bare identifier — treat as 0
        return CSSMathExpr::makeValue(0);
    }

    return CSSMathExpr::makeValue(0);
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::Parser::parseFunction(const std::string& name) {
    std::vector<std::unique_ptr<CSSMathExpr>> args;
    if (!peek(')')) {
        args.push_back(parseExpression());
        while (match(',')) {
            args.push_back(parseExpression());
        }
    }
    match(')');

    CSSMathExpr::Op op = CSSMathExpr::Op::Value;
    if (name == "calc") {
        if (!args.empty()) return std::move(args[0]);
    } else if (name == "min") op = CSSMathExpr::Op::Min;
    else if (name == "max") op = CSSMathExpr::Op::Max;
    else if (name == "clamp") op = CSSMathExpr::Op::Clamp;
    else if (name == "abs") op = CSSMathExpr::Op::Abs;
    else if (name == "sign") op = CSSMathExpr::Op::Sign;
    else if (name == "round") op = CSSMathExpr::Op::Round;
    else if (name == "mod") op = CSSMathExpr::Op::Mod;
    else if (name == "rem") op = CSSMathExpr::Op::Rem;
    else if (name == "sin") op = CSSMathExpr::Op::Sin;
    else if (name == "cos") op = CSSMathExpr::Op::Cos;
    else if (name == "tan") op = CSSMathExpr::Op::Tan;
    else if (name == "asin") op = CSSMathExpr::Op::Asin;
    else if (name == "acos") op = CSSMathExpr::Op::Acos;
    else if (name == "atan") op = CSSMathExpr::Op::Atan;
    else if (name == "atan2") op = CSSMathExpr::Op::Atan2;
    else if (name == "pow") op = CSSMathExpr::Op::Pow;
    else if (name == "sqrt") op = CSSMathExpr::Op::Sqrt;
    else if (name == "hypot") op = CSSMathExpr::Op::Hypot;
    else if (name == "log") op = CSSMathExpr::Op::Log;
    else if (name == "exp") op = CSSMathExpr::Op::Exp;

    return CSSMathExpr::makeFunction(op, std::move(args));
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::parse(const std::string& expr) {
    // Strip outermost function wrapper if present
    std::string inner = expr;
    for (const char* fn : {"calc(", "min(", "max(", "clamp("}) {
        if (inner.find(fn) == 0) {
            inner = inner.substr(std::strlen(fn));
            if (!inner.empty() && inner.back() == ')') inner.pop_back();
            break;
        }
    }

    Parser p{inner, 0};

    // Re-wrap for multi-arg functions
    if (expr.find("min(") == 0 || expr.find("max(") == 0 || expr.find("clamp(") == 0) {
        Parser p2{expr, 0};
        std::string name = p2.readIdentifier();
        if (p2.match('(')) {
            return p2.parseFunction(name);
        }
    }

    return p.parseExpression();
}

// ==================================================================
// CSSCalcEngine — Evaluator
// ==================================================================

float CSSCalcEngine::resolveUnit(const CSSUnitValue& val, const Context& ctx) {
    return val.toPx(ctx.fontSize, ctx.rootFontSize, ctx.viewportWidth,
                     ctx.viewportHeight, ctx.parentSize);
}

float CSSCalcEngine::evaluate(const CSSMathExpr& expr, const Context& ctx) {
    switch (expr.op) {
        case CSSMathExpr::Op::Value:
            return resolveUnit(expr.value, ctx);

        case CSSMathExpr::Op::Add:
            return evaluate(*expr.children[0], ctx) + evaluate(*expr.children[1], ctx);
        case CSSMathExpr::Op::Subtract:
            return evaluate(*expr.children[0], ctx) - evaluate(*expr.children[1], ctx);
        case CSSMathExpr::Op::Multiply:
            return evaluate(*expr.children[0], ctx) * evaluate(*expr.children[1], ctx);
        case CSSMathExpr::Op::Divide: {
            float divisor = evaluate(*expr.children[1], ctx);
            return (divisor != 0) ? evaluate(*expr.children[0], ctx) / divisor : 0;
        }

        case CSSMathExpr::Op::Min: {
            float result = evaluate(*expr.children[0], ctx);
            for (size_t i = 1; i < expr.children.size(); i++) {
                result = std::min(result, evaluate(*expr.children[i], ctx));
            }
            return result;
        }
        case CSSMathExpr::Op::Max: {
            float result = evaluate(*expr.children[0], ctx);
            for (size_t i = 1; i < expr.children.size(); i++) {
                result = std::max(result, evaluate(*expr.children[i], ctx));
            }
            return result;
        }
        case CSSMathExpr::Op::Clamp: {
            if (expr.children.size() >= 3) {
                float lo = evaluate(*expr.children[0], ctx);
                float val = evaluate(*expr.children[1], ctx);
                float hi = evaluate(*expr.children[2], ctx);
                return std::clamp(val, lo, hi);
            }
            return 0;
        }

        case CSSMathExpr::Op::Negate:
            return -evaluate(*expr.children[0], ctx);
        case CSSMathExpr::Op::Abs:
            return std::abs(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Sign: {
            float v = evaluate(*expr.children[0], ctx);
            return (v > 0) ? 1.0f : (v < 0) ? -1.0f : 0.0f;
        }
        case CSSMathExpr::Op::Round: {
            if (expr.children.size() >= 2) {
                float val = evaluate(*expr.children[0], ctx);
                float step = evaluate(*expr.children[1], ctx);
                if (step != 0) return std::round(val / step) * step;
            }
            return std::round(evaluate(*expr.children[0], ctx));
        }
        case CSSMathExpr::Op::Mod: {
            float a = evaluate(*expr.children[0], ctx);
            float b = evaluate(*expr.children[1], ctx);
            return (b != 0) ? std::fmod(a, b) : 0;
        }
        case CSSMathExpr::Op::Rem: {
            float a = evaluate(*expr.children[0], ctx);
            float b = evaluate(*expr.children[1], ctx);
            return (b != 0) ? std::remainder(a, b) : 0;
        }

        case CSSMathExpr::Op::Sin: return std::sin(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Cos: return std::cos(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Tan: return std::tan(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Asin: return std::asin(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Acos: return std::acos(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Atan: return std::atan(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Atan2:
            return std::atan2(evaluate(*expr.children[0], ctx), evaluate(*expr.children[1], ctx));
        case CSSMathExpr::Op::Pow:
            return std::pow(evaluate(*expr.children[0], ctx), evaluate(*expr.children[1], ctx));
        case CSSMathExpr::Op::Sqrt:
            return std::sqrt(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Hypot: {
            float sum = 0;
            for (const auto& c : expr.children) {
                float v = evaluate(*c, ctx);
                sum += v * v;
            }
            return std::sqrt(sum);
        }
        case CSSMathExpr::Op::Log:
            return std::log(evaluate(*expr.children[0], ctx));
        case CSSMathExpr::Op::Exp:
            return std::exp(evaluate(*expr.children[0], ctx));
    }
    return 0;
}

std::unique_ptr<CSSMathExpr> CSSCalcEngine::simplify(std::unique_ptr<CSSMathExpr> expr) {
    if (!expr) return nullptr;

    // Simplify children first
    for (auto& child : expr->children) {
        child = simplify(std::move(child));
    }

    // Constant fold if all children are values with the same unit
    if (expr->children.size() == 2 &&
        expr->children[0]->op == CSSMathExpr::Op::Value &&
        expr->children[1]->op == CSSMathExpr::Op::Value &&
        expr->children[0]->value.unit == expr->children[1]->value.unit) {

        float a = expr->children[0]->value.value;
        float b = expr->children[1]->value.value;
        CSSUnit u = expr->children[0]->value.unit;

        switch (expr->op) {
            case CSSMathExpr::Op::Add: return CSSMathExpr::makeValue(a + b, u);
            case CSSMathExpr::Op::Subtract: return CSSMathExpr::makeValue(a - b, u);
            default: break;
        }
    }

    // Multiply/divide with number
    if (expr->op == CSSMathExpr::Op::Multiply && expr->children.size() == 2) {
        if (expr->children[1]->op == CSSMathExpr::Op::Value &&
            expr->children[1]->value.unit == CSSUnit::Number &&
            expr->children[0]->op == CSSMathExpr::Op::Value) {
            return CSSMathExpr::makeValue(
                expr->children[0]->value.value * expr->children[1]->value.value,
                expr->children[0]->value.unit);
        }
    }

    return expr;
}

std::string CSSCalcEngine::serialize(const CSSMathExpr& expr) {
    switch (expr.op) {
        case CSSMathExpr::Op::Value: {
            std::ostringstream ss;
            ss << expr.value.value;
            switch (expr.value.unit) {
                case CSSUnit::Px: ss << "px"; break;
                case CSSUnit::Em: ss << "em"; break;
                case CSSUnit::Rem: ss << "rem"; break;
                case CSSUnit::Percent: ss << "%"; break;
                case CSSUnit::Vw: ss << "vw"; break;
                case CSSUnit::Vh: ss << "vh"; break;
                default: break;
            }
            return ss.str();
        }
        case CSSMathExpr::Op::Add:
            return serialize(*expr.children[0]) + " + " + serialize(*expr.children[1]);
        case CSSMathExpr::Op::Subtract:
            return serialize(*expr.children[0]) + " - " + serialize(*expr.children[1]);
        case CSSMathExpr::Op::Multiply:
            return serialize(*expr.children[0]) + " * " + serialize(*expr.children[1]);
        case CSSMathExpr::Op::Divide:
            return serialize(*expr.children[0]) + " / " + serialize(*expr.children[1]);
        default: {
            std::string name;
            switch (expr.op) {
                case CSSMathExpr::Op::Min: name = "min"; break;
                case CSSMathExpr::Op::Max: name = "max"; break;
                case CSSMathExpr::Op::Clamp: name = "clamp"; break;
                case CSSMathExpr::Op::Abs: name = "abs"; break;
                case CSSMathExpr::Op::Sqrt: name = "sqrt"; break;
                default: name = "calc"; break;
            }
            std::string result = name + "(";
            for (size_t i = 0; i < expr.children.size(); i++) {
                if (i > 0) result += ", ";
                result += serialize(*expr.children[i]);
            }
            return result + ")";
        }
    }
}

// ==================================================================
// CSSColorValue
// ==================================================================

CSSColorValue CSSColorValue::fromHex(const std::string& hex) {
    CSSColorValue c;
    std::string h = hex;
    if (!h.empty() && h[0] == '#') h = h.substr(1);

    if (h.size() == 3) {
        c.r = static_cast<float>(std::stoi(std::string(2, h[0]), nullptr, 16)) / 255.0f;
        c.g = static_cast<float>(std::stoi(std::string(2, h[1]), nullptr, 16)) / 255.0f;
        c.b = static_cast<float>(std::stoi(std::string(2, h[2]), nullptr, 16)) / 255.0f;
    } else if (h.size() == 6) {
        c.r = static_cast<float>(std::stoi(h.substr(0, 2), nullptr, 16)) / 255.0f;
        c.g = static_cast<float>(std::stoi(h.substr(2, 2), nullptr, 16)) / 255.0f;
        c.b = static_cast<float>(std::stoi(h.substr(4, 2), nullptr, 16)) / 255.0f;
    } else if (h.size() == 8) {
        c.r = static_cast<float>(std::stoi(h.substr(0, 2), nullptr, 16)) / 255.0f;
        c.g = static_cast<float>(std::stoi(h.substr(2, 2), nullptr, 16)) / 255.0f;
        c.b = static_cast<float>(std::stoi(h.substr(4, 2), nullptr, 16)) / 255.0f;
        c.a = static_cast<float>(std::stoi(h.substr(6, 2), nullptr, 16)) / 255.0f;
    }
    return c;
}

CSSColorValue CSSColorValue::fromRGB(int r, int g, int b, float a) {
    return {r / 255.0f, g / 255.0f, b / 255.0f, a};
}

CSSColorValue CSSColorValue::fromHSL(float h, float s, float l, float a) {
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);
    h = std::fmod(h, 360.0f);
    if (h < 0) h += 360;

    float c = (1 - std::abs(2 * l - 1)) * s;
    float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2.0f) - 1));
    float m = l - c / 2;

    float r1 = 0, g1 = 0, b1 = 0;
    if (h < 60) { r1 = c; g1 = x; }
    else if (h < 120) { r1 = x; g1 = c; }
    else if (h < 180) { g1 = c; b1 = x; }
    else if (h < 240) { g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; b1 = c; }
    else { r1 = c; b1 = x; }

    return {r1 + m, g1 + m, b1 + m, a};
}

CSSColorValue CSSColorValue::fromHWB(float h, float w, float bl, float a) {
    auto hsl = fromHSL(h, 1.0f, 0.5f, a);
    float total = w + bl;
    if (total > 1.0f) { w /= total; bl /= total; }
    hsl.r = hsl.r * (1 - w - bl) + w;
    hsl.g = hsl.g * (1 - w - bl) + w;
    hsl.b = hsl.b * (1 - w - bl) + w;
    return hsl;
}

CSSColorValue CSSColorValue::fromOKLCH(float l, float c, float h, float a) {
    // OKLCH → OKLAB → linear sRGB
    float hRad = h * 3.14159265f / 180.0f;
    float aAxis = c * std::cos(hRad);
    float bAxis = c * std::sin(hRad);
    return fromOKLAB(l, aAxis, bAxis, a);
}

CSSColorValue CSSColorValue::fromOKLAB(float l, float a_axis, float b_axis, float alpha) {
    // OKLAB → linear sRGB (approximate)
    float l_ = l + 0.3963377774f * a_axis + 0.2158037573f * b_axis;
    float m_ = l - 0.1055613458f * a_axis - 0.0638541728f * b_axis;
    float s_ = l - 0.0894841775f * a_axis - 1.2914855480f * b_axis;

    float ll = l_ * l_ * l_;
    float mm = m_ * m_ * m_;
    float ss = s_ * s_ * s_;

    float r = 4.0767416621f * ll - 3.3077115913f * mm + 0.2309699292f * ss;
    float g = -1.2684380046f * ll + 2.6097574011f * mm - 0.3413193965f * ss;
    float b = -0.0041960863f * ll - 0.7034186147f * mm + 1.7076147010f * ss;

    return {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f),
            std::clamp(b, 0.0f, 1.0f), alpha};
}

void CSSColorValue::toHSL(float& h, float& s, float& l) const {
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    l = (mx + mn) / 2;

    if (mx == mn) { h = s = 0; return; }

    float d = mx - mn;
    s = l > 0.5f ? d / (2 - mx - mn) : d / (mx + mn);

    if (mx == r) h = std::fmod((g - b) / d + (g < b ? 6.0f : 0.0f), 6.0f) * 60.0f;
    else if (mx == g) h = ((b - r) / d + 2) * 60.0f;
    else h = ((r - g) / d + 4) * 60.0f;
}

void CSSColorValue::toOKLCH(float& ll, float& c, float& h) const {
    // Approximate sRGB → OKLAB → OKLCH
    float l_ = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    float m_ = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    float s_ = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    l_ = std::cbrt(l_);
    m_ = std::cbrt(m_);
    s_ = std::cbrt(s_);

    ll = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    float a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    float bAxis = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;

    c = std::sqrt(a * a + bAxis * bAxis);
    h = std::atan2(bAxis, a) * 180.0f / 3.14159265f;
    if (h < 0) h += 360;
}

std::string CSSColorValue::toHexString() const {
    char buf[10];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x",
             static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
    return std::string(buf);
}

std::string CSSColorValue::toRGBString() const {
    std::ostringstream ss;
    ss << "rgb(" << static_cast<int>(r * 255) << ", "
       << static_cast<int>(g * 255) << ", "
       << static_cast<int>(b * 255);
    if (a < 1.0f) ss << " / " << a;
    ss << ")";
    return ss.str();
}

CSSColorValue CSSColorValue::mix(const CSSColorValue& c1, const CSSColorValue& c2,
                                   float amount, const std::string& colorSpace) {
    return interpolate(c1, c2, amount, colorSpace);
}

CSSColorValue CSSColorValue::interpolate(const CSSColorValue& from, const CSSColorValue& to,
                                            float t, const std::string& colorSpace) {
    if (colorSpace == "oklch") {
        float fl, fc, fh, tl, tc, th;
        from.toOKLCH(fl, fc, fh);
        to.toOKLCH(tl, tc, th);

        // Hue interpolation (shorter arc)
        float hDiff = th - fh;
        if (hDiff > 180) hDiff -= 360;
        if (hDiff < -180) hDiff += 360;

        float l = fl + (tl - fl) * t;
        float c = fc + (tc - fc) * t;
        float h = fh + hDiff * t;
        float a = from.a + (to.a - from.a) * t;

        return fromOKLCH(l, c, h, a);
    }

    // sRGB interpolation
    return {
        from.r + (to.r - from.r) * t,
        from.g + (to.g - from.g) * t,
        from.b + (to.b - from.b) * t,
        from.a + (to.a - from.a) * t
    };
}

bool CSSColorValue::isNamedColor(const std::string& name) {
    static const std::unordered_map<std::string, int> named = {
        {"black", 1}, {"white", 1}, {"red", 1}, {"green", 1}, {"blue", 1},
        {"yellow", 1}, {"cyan", 1}, {"magenta", 1}, {"orange", 1}, {"purple", 1},
        {"pink", 1}, {"brown", 1}, {"gray", 1}, {"grey", 1}, {"silver", 1},
        {"gold", 1}, {"navy", 1}, {"teal", 1}, {"olive", 1}, {"maroon", 1},
        {"aqua", 1}, {"lime", 1}, {"fuchsia", 1}, {"transparent", 1},
        {"currentcolor", 1}, {"inherit", 1},
    };
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return named.count(lower) > 0;
}

CSSColorValue CSSColorValue::fromName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "black") return fromRGB(0, 0, 0);
    if (lower == "white") return fromRGB(255, 255, 255);
    if (lower == "red") return fromRGB(255, 0, 0);
    if (lower == "green") return fromRGB(0, 128, 0);
    if (lower == "blue") return fromRGB(0, 0, 255);
    if (lower == "yellow") return fromRGB(255, 255, 0);
    if (lower == "cyan" || lower == "aqua") return fromRGB(0, 255, 255);
    if (lower == "magenta" || lower == "fuchsia") return fromRGB(255, 0, 255);
    if (lower == "orange") return fromRGB(255, 165, 0);
    if (lower == "purple") return fromRGB(128, 0, 128);
    if (lower == "pink") return fromRGB(255, 192, 203);
    if (lower == "brown") return fromRGB(165, 42, 42);
    if (lower == "gray" || lower == "grey") return fromRGB(128, 128, 128);
    if (lower == "silver") return fromRGB(192, 192, 192);
    if (lower == "gold") return fromRGB(255, 215, 0);
    if (lower == "navy") return fromRGB(0, 0, 128);
    if (lower == "teal") return fromRGB(0, 128, 128);
    if (lower == "olive") return fromRGB(128, 128, 0);
    if (lower == "maroon") return fromRGB(128, 0, 0);
    if (lower == "lime") return fromRGB(0, 255, 0);
    if (lower == "transparent") return {0, 0, 0, 0};
    return fromRGB(0, 0, 0);
}

CSSColorValue CSSColorValue::parse(const std::string& str) {
    if (str.empty()) return fromRGB(0, 0, 0);
    if (str[0] == '#') return fromHex(str);
    if (isNamedColor(str)) return fromName(str);

    // rgb(r, g, b) or rgba(r, g, b, a)
    if (str.find("rgb") == 0) {
        auto paren = str.find('(');
        auto end = str.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string inner = str.substr(paren + 1, end - paren - 1);
            int r = 0, g = 0, b = 0;
            float a = 1.0f;
            if (sscanf(inner.c_str(), "%d,%d,%d,%f", &r, &g, &b, &a) >= 3 ||
                sscanf(inner.c_str(), "%d %d %d / %f", &r, &g, &b, &a) >= 3) {
                return fromRGB(r, g, b, a);
            }
        }
    }

    // hsl(h, s%, l%) or hsla(h, s%, l%, a)
    if (str.find("hsl") == 0) {
        auto paren = str.find('(');
        auto end = str.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string inner = str.substr(paren + 1, end - paren - 1);
            float h = 0, s = 0, l = 0, a = 1.0f;
            if (sscanf(inner.c_str(), "%f,%f%%,%f%%,%f", &h, &s, &l, &a) >= 3 ||
                sscanf(inner.c_str(), "%f %f%% %f%% / %f", &h, &s, &l, &a) >= 3) {
                return fromHSL(h, s / 100.0f, l / 100.0f, a);
            }
        }
    }

    return fromRGB(0, 0, 0);
}

} // namespace Web
} // namespace NXRender
