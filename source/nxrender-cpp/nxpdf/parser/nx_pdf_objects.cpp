// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf_objects.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace nxrender {
namespace pdf {
namespace parser {

// ==================================================================
// Type name resolution
// ==================================================================

const char* PdfObjectTypeName(PdfObjectType type) {
    switch (type) {
        case PdfObjectType::Null:       return "null";
        case PdfObjectType::Bool:       return "boolean";
        case PdfObjectType::Integer:    return "integer";
        case PdfObjectType::Real:       return "real";
        case PdfObjectType::Name:       return "name";
        case PdfObjectType::String:     return "string";
        case PdfObjectType::Array:      return "array";
        case PdfObjectType::Dictionary: return "dictionary";
        case PdfObjectType::Stream:     return "stream";
        case PdfObjectType::Reference:  return "reference";
    }
    return "unknown";
}

// ==================================================================
// Serialization
// ==================================================================

std::string SerializeObject(const PdfObject* obj) {
    if (!obj) return "null";

    switch (obj->GetType()) {
        case PdfObjectType::Null:
            return "null";

        case PdfObjectType::Bool: {
            auto* b = static_cast<const PdfBool*>(obj);
            return b->value ? "true" : "false";
        }

        case PdfObjectType::Integer: {
            auto* i = static_cast<const PdfInteger*>(obj);
            return std::to_string(i->value);
        }

        case PdfObjectType::Real: {
            auto* r = static_cast<const PdfReal*>(obj);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << r->value;
            std::string s = oss.str();
            // Trim trailing zeros
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last = s.find_last_not_of('0');
                if (last == dot) last++;
                s = s.substr(0, last + 1);
            }
            return s;
        }

        case PdfObjectType::Name: {
            auto* n = static_cast<const PdfName*>(obj);
            return "/" + n->value;
        }

        case PdfObjectType::String: {
            auto* s = static_cast<const PdfString*>(obj);
            return "(" + s->value + ")";
        }

        case PdfObjectType::Array: {
            auto* arr = static_cast<const PdfArray*>(obj);
            std::string result = "[";
            for (size_t i = 0; i < arr->items.size(); i++) {
                if (i > 0) result += " ";
                result += SerializeObject(arr->items[i].get());
            }
            result += "]";
            return result;
        }

        case PdfObjectType::Dictionary: {
            auto* dict = static_cast<const PdfDictionary*>(obj);
            std::string result = "<<\n";
            for (const auto& pair : dict->map) {
                result += "/" + pair.first + " " +
                          SerializeObject(pair.second.get()) + "\n";
            }
            result += ">>";
            return result;
        }

        case PdfObjectType::Stream: {
            auto* stream = static_cast<const PdfStream*>(obj);
            std::string result = SerializeObject(stream->dictionary.get());
            result += "\nstream\n";
            result += std::string(reinterpret_cast<const char*>(stream->data.data()),
                                  stream->data.size());
            result += "\nendstream";
            return result;
        }

        case PdfObjectType::Reference: {
            auto* ref = static_cast<const PdfReference*>(obj);
            return std::to_string(ref->objectNumber) + " " +
                   std::to_string(ref->generationNumber) + " R";
        }
    }
    return "null";
}

// ==================================================================
// Dictionary helpers
// ==================================================================

PdfObject* DictGet(const PdfDictionary* dict, const std::string& key) {
    if (!dict) return nullptr;
    auto it = dict->map.find(key);
    if (it == dict->map.end()) return nullptr;
    return it->second.get();
}

int DictGetInt(const PdfDictionary* dict, const std::string& key, int defaultVal) {
    auto* obj = DictGet(dict, key);
    if (!obj) return defaultVal;
    if (auto* i = dynamic_cast<PdfInteger*>(obj)) return i->value;
    if (auto* r = dynamic_cast<PdfReal*>(obj)) return static_cast<int>(r->value);
    return defaultVal;
}

double DictGetReal(const PdfDictionary* dict, const std::string& key, double defaultVal) {
    auto* obj = DictGet(dict, key);
    if (!obj) return defaultVal;
    if (auto* r = dynamic_cast<PdfReal*>(obj)) return r->value;
    if (auto* i = dynamic_cast<PdfInteger*>(obj)) return static_cast<double>(i->value);
    return defaultVal;
}

std::string DictGetName(const PdfDictionary* dict, const std::string& key) {
    auto* obj = DictGet(dict, key);
    if (!obj) return "";
    if (auto* n = dynamic_cast<PdfName*>(obj)) return n->value;
    return "";
}

std::string DictGetString(const PdfDictionary* dict, const std::string& key) {
    auto* obj = DictGet(dict, key);
    if (!obj) return "";
    if (auto* s = dynamic_cast<PdfString*>(obj)) return s->value;
    return "";
}

bool DictGetBool(const PdfDictionary* dict, const std::string& key, bool defaultVal) {
    auto* obj = DictGet(dict, key);
    if (!obj) return defaultVal;
    if (auto* b = dynamic_cast<PdfBool*>(obj)) return b->value;
    return defaultVal;
}

bool DictHasKey(const PdfDictionary* dict, const std::string& key) {
    if (!dict) return false;
    return dict->map.find(key) != dict->map.end();
}

// ==================================================================
// Deep clone
// ==================================================================

std::unique_ptr<PdfObject> CloneObject(const PdfObject* obj) {
    if (!obj) return std::make_unique<PdfNull>();

    switch (obj->GetType()) {
        case PdfObjectType::Null:
            return std::make_unique<PdfNull>();
        case PdfObjectType::Bool:
            return std::make_unique<PdfBool>(
                static_cast<const PdfBool*>(obj)->value);
        case PdfObjectType::Integer:
            return std::make_unique<PdfInteger>(
                static_cast<const PdfInteger*>(obj)->value);
        case PdfObjectType::Real:
            return std::make_unique<PdfReal>(
                static_cast<const PdfReal*>(obj)->value);
        case PdfObjectType::Name:
            return std::make_unique<PdfName>(
                static_cast<const PdfName*>(obj)->value);
        case PdfObjectType::String:
            return std::make_unique<PdfString>(
                static_cast<const PdfString*>(obj)->value);
        case PdfObjectType::Array: {
            auto clone = std::make_unique<PdfArray>();
            auto* arr = static_cast<const PdfArray*>(obj);
            for (const auto& item : arr->items) {
                clone->items.push_back(CloneObject(item.get()));
            }
            return clone;
        }
        case PdfObjectType::Dictionary: {
            auto clone = std::make_unique<PdfDictionary>();
            auto* dict = static_cast<const PdfDictionary*>(obj);
            for (const auto& pair : dict->map) {
                clone->map[pair.first] = CloneObject(pair.second.get());
            }
            return clone;
        }
        case PdfObjectType::Stream: {
            auto* s = static_cast<const PdfStream*>(obj);
            auto dictClone = std::unique_ptr<PdfDictionary>(
                static_cast<PdfDictionary*>(CloneObject(s->dictionary.get()).release()));
            return std::make_unique<PdfStream>(std::move(dictClone), s->data);
        }
        case PdfObjectType::Reference: {
            auto* ref = static_cast<const PdfReference*>(obj);
            return std::make_unique<PdfReference>(ref->objectNumber, ref->generationNumber);
        }
    }
    return std::make_unique<PdfNull>();
}

} // namespace parser
} // namespace pdf
} // namespace nxrender
