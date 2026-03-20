// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "engine/extension.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace ZepraEngine {

class ExtensionRegistry {
public:
    struct ExtensionHandle {
        std::string path;
        std::unique_ptr<IExtension> instance;
#ifdef _WIN32
        HMODULE handle;
#else
        void* handle;
#endif
        bool enabled = false;
    };

    bool loadExtension(const std::string& path) {
#ifdef _WIN32
        HMODULE lib = LoadLibraryA(path.c_str());
        if (!lib) return false;
        auto create = (IExtension*(*)())GetProcAddress(lib, "createExtension");
#else
        void* lib = dlopen(path.c_str(), RTLD_LAZY);
        if (!lib) return false;
        auto create = (IExtension*(*)())dlsym(lib, "createExtension");
#endif
        if (!create) {
#ifdef _WIN32
            FreeLibrary(lib);
#else
            dlclose(lib);
#endif
            return false;
        }
        std::unique_ptr<IExtension> ext(create());
        ext->onLoad();
        ExtensionHandle handle{path, std::move(ext), lib, false};
        m_extensions[handle.instance->getName()] = std::move(handle);
        std::cout << "[ExtensionRegistry] Loaded: " << path << std::endl;
        return true;
    }

    void enableExtension(const std::string& name) {
        auto it = m_extensions.find(name);
        if (it != m_extensions.end() && !it->second.enabled) {
            it->second.instance->onEnable();
            it->second.enabled = true;
            std::cout << "[ExtensionRegistry] Enabled: " << name << std::endl;
        }
    }

    void disableExtension(const std::string& name) {
        auto it = m_extensions.find(name);
        if (it != m_extensions.end() && it->second.enabled) {
            it->second.instance->onDisable();
            it->second.enabled = false;
            std::cout << "[ExtensionRegistry] Disabled: " << name << std::endl;
        }
    }

    void unloadExtension(const std::string& name) {
        auto it = m_extensions.find(name);
        if (it != m_extensions.end()) {
            it->second.instance->onUnload();
#ifdef _WIN32
            FreeLibrary(it->second.handle);
#else
            dlclose(it->second.handle);
#endif
            m_extensions.erase(it);
            std::cout << "[ExtensionRegistry] Unloaded: " << name << std::endl;
        }
    }

    std::vector<std::string> listExtensions() const {
        std::vector<std::string> names;
        for (const auto& kv : m_extensions) {
            names.push_back(kv.first);
        }
        return names;
    }

private:
    std::unordered_map<std::string, ExtensionHandle> m_extensions;
};

} // namespace ZepraEngine 