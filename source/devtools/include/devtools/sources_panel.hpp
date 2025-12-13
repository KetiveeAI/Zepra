/**
 * @file sources_panel.hpp
 * @brief Sources/Debugger Panel - JavaScript debugging
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Zepra::DevTools {

/**
 * @brief Source file info
 */
struct SourceFile {
    int scriptId;
    std::string url;
    std::string filename;
    std::string content;
    int lineCount;
    std::string sourceMapUrl;
    bool isInlineScript;
};

/**
 * @brief Breakpoint info
 */
struct Breakpoint {
    int id;
    std::string url;
    int line;
    int column;
    std::string condition;
    bool enabled;
    bool resolved;
    int hitCount;
};

/**
 * @brief Call frame (stack frame)
 */
struct CallFrame {
    int index;
    std::string functionName;
    std::string url;
    int line;
    int column;
    bool isAsync;
};

/**
 * @brief Variable/Scope info
 */
struct ScopeVariable {
    std::string name;
    std::string value;
    std::string type;
    bool expandable;
    bool writable;
    std::vector<ScopeVariable> properties;
};

struct Scope {
    enum class Type { Local, Closure, Block, Global, With, Catch } type;
    std::string name;
    std::vector<ScopeVariable> variables;
};

/**
 * @brief Debugger state
 */
enum class DebuggerState {
    Running,
    Paused,
    Stepping
};

/**
 * @brief Pause reason
 */
enum class PauseReason {
    Breakpoint,
    Step,
    Exception,
    DebuggerStatement,
    Other
};

/**
 * @brief Debugger callbacks
 */
using PausedCallback = std::function<void(PauseReason, const std::vector<CallFrame>&)>;
using ResumedCallback = std::function<void()>;
using ScriptParsedCallback = std::function<void(const SourceFile&)>;

/**
 * @brief Sources Panel - JavaScript Debugger
 */
class SourcesPanel {
public:
    SourcesPanel();
    ~SourcesPanel();
    
    // --- Sources ---
    
    /**
     * @brief Add a source file
     */
    void addSource(const SourceFile& source);
    
    /**
     * @brief Get all sources
     */
    const std::vector<SourceFile>& sources() const { return sources_; }
    
    /**
     * @brief Get source by script ID
     */
    const SourceFile* getSource(int scriptId) const;
    
    /**
     * @brief Open source file
     */
    void openSource(int scriptId, int line = 1);
    int currentSource() const { return currentScriptId_; }
    
    // --- Breakpoints ---
    
    /**
     * @brief Set breakpoint
     */
    int setBreakpoint(const std::string& url, int line, 
                      const std::string& condition = "");
    
    /**
     * @brief Remove breakpoint
     */
    bool removeBreakpoint(int id);
    
    /**
     * @brief Toggle breakpoint
     */
    void toggleBreakpoint(int id);
    
    /**
     * @brief Get all breakpoints
     */
    const std::vector<Breakpoint>& breakpoints() const { return breakpoints_; }
    
    /**
     * @brief Clear all breakpoints
     */
    void clearAllBreakpoints();
    
    // --- Execution Control ---
    
    /**
     * @brief Resume execution
     */
    void resume();
    
    /**
     * @brief Step over
     */
    void stepOver();
    
    /**
     * @brief Step into
     */
    void stepInto();
    
    /**
     * @brief Step out
     */
    void stepOut();
    
    /**
     * @brief Pause execution
     */
    void pause();
    
    /**
     * @brief Get debugger state
     */
    DebuggerState state() const { return state_; }
    bool isPaused() const { return state_ == DebuggerState::Paused; }
    
    // --- Call Stack ---
    
    /**
     * @brief Get call stack (when paused)
     */
    const std::vector<CallFrame>& callStack() const { return callStack_; }
    
    /**
     * @brief Select call frame
     */
    void selectFrame(int index);
    int selectedFrame() const { return selectedFrameIndex_; }
    
    // --- Scopes ---
    
    /**
     * @brief Get scopes for selected frame
     */
    std::vector<Scope> getScopes();
    
    /**
     * @brief Evaluate expression in selected frame
     */
    ScopeVariable evaluate(const std::string& expression);
    
    // --- Watch Expressions ---
    
    /**
     * @brief Add watch expression
     */
    void addWatch(const std::string& expression);
    
    /**
     * @brief Remove watch expression
     */
    void removeWatch(const std::string& expression);
    
    /**
     * @brief Get watch expressions and values
     */
    std::vector<std::pair<std::string, ScopeVariable>> getWatches();
    
    // --- Settings ---
    
    /**
     * @brief Pause on exceptions
     */
    void setPauseOnExceptions(bool all, bool uncaught);
    
    /**
     * @brief Blackbox a script (skip when stepping)
     */
    void blackboxScript(const std::string& url);
    void unblackboxScript(const std::string& url);
    
    // --- Callbacks ---
    void onPaused(PausedCallback callback);
    void onResumed(ResumedCallback callback);
    void onScriptParsed(ScriptParsedCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
private:
    void renderSourceTree();
    void renderEditor();
    void renderBreakpoints();
    void renderCallStack();
    void renderScopes();
    void renderWatches();
    
    std::vector<SourceFile> sources_;
    std::vector<Breakpoint> breakpoints_;
    std::vector<CallFrame> callStack_;
    std::vector<std::string> watches_;
    std::unordered_map<std::string, bool> blackboxed_;
    
    int currentScriptId_ = -1;
    int currentLine_ = 1;
    int selectedFrameIndex_ = 0;
    int nextBreakpointId_ = 1;
    
    DebuggerState state_ = DebuggerState::Running;
    bool pauseOnAllExceptions_ = false;
    bool pauseOnUncaughtExceptions_ = true;
    
    std::vector<PausedCallback> pausedCallbacks_;
    std::vector<ResumedCallback> resumedCallbacks_;
    std::vector<ScriptParsedCallback> scriptCallbacks_;
};

} // namespace Zepra::DevTools
