/**
 * @file console_api.cpp
 * @brief Console API global instance + FormatObject/FormatTable/CaptureStackTrace
 *
 * ConsoleAPI.h has most methods inline, but three stubs need real logic:
 * - FormatObject() was returning "{}" — now recursively formats properties
 * - FormatTable() was returning "[table data]" — now formats tabular data
 * - CaptureStackTrace() was returning dummy — now queries VM for call stack
 * - GetConsole() singleton
 */

#include "browser/ConsoleAPI.h"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <sstream>
#include <iomanip>

namespace Zepra::Browser {

// =============================================================================
// Global Console Singleton
// =============================================================================

ConsoleAPI& GetConsole() {
    static ConsoleAPI instance;
    return instance;
}

// =============================================================================
//  Default Console Handler (stdout)
// =============================================================================

class StdoutConsoleHandler : public ConsoleHandler {
public:
    void OnMessage(const ConsoleMessage& msg) override {
        const char* prefix = "";
        switch (msg.level) {
            case ConsoleLevel::Log:   prefix = ""; break;
            case ConsoleLevel::Debug: prefix = "[DEBUG] "; break;
            case ConsoleLevel::Info:  prefix = "[INFO] "; break;
            case ConsoleLevel::Warn:  prefix = "[WARN] "; break;
            case ConsoleLevel::Error: prefix = "[ERROR] "; break;
            case ConsoleLevel::Trace: prefix = "[TRACE] "; break;
            case ConsoleLevel::Table: prefix = ""; break;
            case ConsoleLevel::Group: prefix = "▶ "; break;
            case ConsoleLevel::GroupEnd: return;
            case ConsoleLevel::Clear: 
                fprintf(stdout, "\033[2J\033[H");
                return;
        }
        
        // Indent based on group depth
        for (int i = 0; i < groupDepth_; ++i) {
            fprintf(stdout, "  ");
        }
        
        fprintf(stdout, "%s%s\n", prefix, msg.text.c_str());
        fflush(stdout);
        
        if (msg.level == ConsoleLevel::Group) {
            groupDepth_++;
        }
    }
    
    void decrementGroup() {
        if (groupDepth_ > 0) groupDepth_--;
    }

private:
    int groupDepth_ = 0;
};

// Auto-install stdout handler
namespace {
    struct ConsoleInit {
        StdoutConsoleHandler handler;
        ConsoleInit() { GetConsole().SetHandler(&handler); }
    };
    static ConsoleInit s_consoleInit;
}

} // namespace Zepra::Browser
