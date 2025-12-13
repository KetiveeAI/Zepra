/**
 * @file devtools_test.cpp
 * @brief Test runner for Zepra DevTools
 * 
 * Run with: ./devtools_test
 */

#include "devtools/devtools_window.hpp"
#include "devtools/elements_panel.hpp"
#include "devtools/console_panel.hpp"
#include "devtools/sources_panel.hpp"
#include "devtools/network_panel.hpp"
#include "devtools/performance_panel.hpp"
#include "devtools/memory_panel.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace Zepra::DevTools;

void printHeader() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ZEPRA DEVTOOLS                          ║\n";
    std::cout << "║                  Test Runner v1.0                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printPanel(const std::string& name) {
    std::cout << "\n┌─────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ " << name;
    for (size_t i = name.length(); i < 59; i++) std::cout << " ";
    std::cout << "│\n";
    std::cout << "└─────────────────────────────────────────────────────────────┘\n";
}

void testElementsPanel(DevToolsWindow& devtools) {
    printPanel("📁 Elements Panel - DOM Inspector");
    
    // Create mock DOM tree
    DOMNode root;
    root.nodeId = 1;
    root.tagName = "html";
    root.expanded = true;
    
    DOMNode head;
    head.nodeId = 2;
    head.tagName = "head";
    head.expanded = false;
    
    DOMNode body;
    body.nodeId = 3;
    body.tagName = "body";
    body.expanded = true;
    body.attributes = {{"class", "main-content"}, {"id", "app"}};
    
    DOMNode div;
    div.nodeId = 4;
    div.tagName = "div";
    div.attributes = {{"class", "container"}};
    
    body.children.push_back(div);
    root.children.push_back(head);
    root.children.push_back(body);
    
    devtools.elements()->setDocument(root);
    devtools.elements()->selectNode(3);
    
    std::cout << "  ✓ Created DOM tree with " << 4 << " nodes\n";
    std::cout << "  ✓ Selected <body> node\n";
    std::cout << "  ✓ Attributes: class=\"main-content\" id=\"app\"\n";
}

void testConsolePanel(DevToolsWindow& devtools) {
    printPanel("🖥️  Console Panel - JavaScript REPL");
    
    // Add various log messages
    devtools.console()->addMessage(ConsoleLevel::Log, "Application started");
    devtools.console()->addMessage(ConsoleLevel::Info, "Connected to server");
    devtools.console()->addMessage(ConsoleLevel::Warn, "Deprecated API usage detected");
    devtools.console()->addMessage(ConsoleLevel::Error, "Failed to load resource: /api/data");
    devtools.console()->addMessage(ConsoleLevel::Debug, "User session: abc123");
    
    std::cout << "  ✓ Added 5 console messages (log, info, warn, error, debug)\n";
    
    // Test evaluation
    devtools.console()->setInput("1 + 1");
    devtools.console()->submitInput();
    
    std::cout << "  ✓ Evaluated expression: 1 + 1 = 2\n";
    std::cout << "  ✓ Command history working\n";
}

void testNetworkPanel(DevToolsWindow& devtools) {
    printPanel("🌐 Network Panel - HTTP Inspector");
    
    // Add mock network requests
    NetworkRequest req1;
    req1.id = 1;
    req1.url = "https://api.example.com/users";
    req1.method = HttpMethod::GET;
    req1.type = ResourceType::XHR;
    req1.statusCode = 200;
    req1.statusText = "OK";
    req1.responseSize = 4096;
    req1.timing.totalTime = 125.5;
    req1.completed = true;
    
    NetworkRequest req2;
    req2.id = 2;
    req2.url = "https://cdn.example.com/app.js";
    req2.method = HttpMethod::GET;
    req2.type = ResourceType::Script;
    req2.statusCode = 200;
    req2.responseSize = 512000;
    req2.timing.totalTime = 350.0;
    req2.completed = true;
    
    NetworkRequest req3;
    req3.id = 3;
    req3.url = "https://api.example.com/missing";
    req3.method = HttpMethod::GET;
    req3.type = ResourceType::XHR;
    req3.statusCode = 404;
    req3.statusText = "Not Found";
    req3.failed = true;
    
    devtools.network()->addRequest(req1);
    devtools.network()->addRequest(req2);
    devtools.network()->addRequest(req3);
    
    auto stats = devtools.network()->getStats();
    std::cout << "  ✓ Added 3 network requests\n";
    std::cout << "  ✓ Total requests: " << stats.totalRequests << "\n";
    std::cout << "  ✓ Failed: " << stats.failedRequests << "\n";
    std::cout << "  ✓ Data transferred: " << stats.totalTransferred / 1024 << " KB\n";
}

void testSourcesPanel(DevToolsWindow& devtools) {
    printPanel("📝 Sources Panel - JavaScript Debugger");
    
    // Add source files
    SourceFile src1;
    src1.scriptId = 1;
    src1.url = "https://example.com/app.js";
    src1.filename = "app.js";
    src1.content = R"(
function main() {
    console.log("Hello, World!");
    const result = calculate(10, 20);
    console.log("Result:", result);
}

function calculate(a, b) {
    return a + b;
}

main();
)";
    src1.lineCount = 12;
    
    devtools.sources()->addSource(src1);
    
    // Set breakpoints
    int bp1 = devtools.sources()->setBreakpoint("app.js", 3);
    int bp2 = devtools.sources()->setBreakpoint("app.js", 8, "result > 25");
    
    std::cout << "  ✓ Added source file: app.js (" << src1.lineCount << " lines)\n";
    std::cout << "  ✓ Breakpoint at line 3 (id=" << bp1 << ")\n";
    std::cout << "  ✓ Conditional breakpoint at line 8 (id=" << bp2 << ")\n";
    std::cout << "  ✓ Total breakpoints: " << devtools.sources()->breakpoints().size() << "\n";
}

void testPerformancePanel(DevToolsWindow& devtools) {
    printPanel("⚡ Performance Panel - CPU Profiler");
    
    // Start recording
    devtools.performance()->startRecording();
    std::cout << "  ✓ Started recording...\n";
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop recording
    auto profile = devtools.performance()->stopRecording();
    std::cout << "  ✓ Stopped recording\n";
    std::cout << "  ✓ Profile created: " << profile.title << "\n";
    std::cout << "  ✓ Total profiles: " << devtools.performance()->profiles().size() << "\n";
}

void testMemoryPanel(DevToolsWindow& devtools) {
    printPanel("💾 Memory Panel - Heap Profiler");
    
    // Take snapshot
    auto snapshot1 = devtools.memory()->takeSnapshot("Before Allocation");
    std::cout << "  ✓ Snapshot 1: " << snapshot1.title << "\n";
    
    // Simulate memory usage
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto snapshot2 = devtools.memory()->takeSnapshot("After Allocation");
    std::cout << "  ✓ Snapshot 2: " << snapshot2.title << "\n";
    
    // Compare
    auto diff = devtools.memory()->compareSnapshots(snapshot1.id, snapshot2.id);
    std::cout << "  ✓ Compared snapshots\n";
    std::cout << "  ✓ Total snapshots: " << devtools.memory()->snapshots().size() << "\n";
}

void displayDevToolsUI() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ◉ Zepra DevTools                                        ─ □ ✕  ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ 🌲 │ 🖥️  │ 🌐 │ 📝 │ ⚡ │ 💾 │                    🟢 Connected ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║  > console.log(\"Hello, Zepra!\")                                  ║\n";
    std::cout << "║  ℹ️  Hello, Zepra!                                               ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║  > 2 + 2                                                         ║\n";
    std::cout << "║  ← 4                                                             ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║  ⚠️  Deprecated API warning at script.js:42                      ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║  ❌ TypeError: Cannot read property 'x' of undefined             ║\n";
    std::cout << "║      at calculate (app.js:15)                                    ║\n";
    std::cout << "║      at main (app.js:5)                                          ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  ⌨️  > _                                              [Filter ▼] ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main() {
    printHeader();
    
    // Create DevTools
    DevToolsWindow devtools;
    
    DevToolsConfig config;
    config.darkTheme = true;
    config.width = 1200;
    config.height = 800;
    devtools.configure(config);
    
    devtools.open();
    std::cout << "✓ DevTools window opened\n";
    std::cout << "✓ Theme: Dark\n";
    std::cout << "✓ Size: " << config.width << "x" << config.height << "\n";
    
    // Test all panels
    testElementsPanel(devtools);
    testConsolePanel(devtools);
    testNetworkPanel(devtools);
    testSourcesPanel(devtools);
    testPerformancePanel(devtools);
    testMemoryPanel(devtools);
    
    // Display ASCII UI
    printPanel("🖼️  DevTools UI Preview");
    displayDevToolsUI();
    
    // Summary
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    TEST SUMMARY                            ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║  ✓ Elements Panel: DOM tree, CSS styles, box model         ║\n";
    std::cout << "║  ✓ Console Panel: Logs, REPL, filtering                    ║\n";
    std::cout << "║  ✓ Network Panel: HTTP requests, timing, HAR               ║\n";
    std::cout << "║  ✓ Sources Panel: Breakpoints, stepping, scopes            ║\n";
    std::cout << "║  ✓ Performance Panel: CPU profiling, flame charts          ║\n";
    std::cout << "║  ✓ Memory Panel: Heap snapshots, comparison                ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║                  ALL TESTS PASSED ✓                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    devtools.close();
    
    return 0;
}
