/**
 * @file resource_loader.cpp
 * @brief Resource loader implementation
 */

#include "webcore/resource_loader.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace Zepra::WebCore {

// =============================================================================
// Headers
// =============================================================================

void Headers::set(const std::string& name, const std::string& value) {
    // Normalize header name to lowercase
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    headers_[normalized] = value;
}

std::string Headers::get(const std::string& name) const {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    auto it = headers_.find(normalized);
    return it != headers_.end() ? it->second : "";
}

bool Headers::has(const std::string& name) const {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return headers_.find(normalized) != headers_.end();
}

void Headers::remove(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    headers_.erase(normalized);
}

// =============================================================================
// Request
// =============================================================================

Request::Request(const std::string& url, HttpMethod method)
    : url_(url), method_(method) {
    // Set default headers
    headers_.set("User-Agent", "ZepraBrowser/1.0");
    headers_.set("Accept", "*/*");
}

// =============================================================================
// Response
// =============================================================================

Response::Response() : status_(0) {}

Response::Response(int status, const std::string& statusText)
    : status_(status), statusText_(statusText) {
    if (statusText_.empty()) {
        switch (status) {
            case 200: statusText_ = "OK"; break;
            case 201: statusText_ = "Created"; break;
            case 204: statusText_ = "No Content"; break;
            case 301: statusText_ = "Moved Permanently"; break;
            case 302: statusText_ = "Found"; break;
            case 304: statusText_ = "Not Modified"; break;
            case 400: statusText_ = "Bad Request"; break;
            case 401: statusText_ = "Unauthorized"; break;
            case 403: statusText_ = "Forbidden"; break;
            case 404: statusText_ = "Not Found"; break;
            case 500: statusText_ = "Internal Server Error"; break;
            default: statusText_ = "Unknown"; break;
        }
    }
}

std::string Response::text() const {
    return std::string(body_.begin(), body_.end());
}

void Response::setBody(const std::string& text) {
    body_.assign(text.begin(), text.end());
}

// =============================================================================
// ResourceLoader
// =============================================================================

ResourceLoader::ResourceLoader() {
    networkLoader_ = std::make_unique<Networking::ResourceLoader>();
}

ResourceLoader::~ResourceLoader() {
    cancelAll();
}

Response ResourceLoader::load(const Request& request) {
    // Internal URLs
    if (request.url().find("zepra://") == 0) {
        Response response(200, "OK");
        response.setUrl(request.url());
        
        if (request.url() == "zepra://start" || request.url() == "zepra://newtab") {
            response.setBody(R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>New Tab</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600&display=swap');
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: linear-gradient(180deg, #e8b4d8 0%, #d4a5e0 25%, #c9a5eb 50%, #b8a5f0 75%, #a8a0f5 100%);
            font-family: 'Inter', 'Segoe UI', -apple-system, sans-serif;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            position: relative;
            overflow: hidden;
        }
        .logo-container {
            margin-bottom: 50px;
            text-align: center;
            animation: fadeInDown 0.8s ease-out;
        }
        @keyframes fadeInDown {
            from { opacity: 0; transform: translateY(-30px); }
            to { opacity: 1; transform: translateY(0); }
        }
        @keyframes fadeInUp {
            from { opacity: 0; transform: translateY(30px); }
            to { opacity: 1; transform: translateY(0); }
        }
        @keyframes float {
            0%, 100% { transform: translateY(0px) rotate(0deg); }
            50% { transform: translateY(-15px) rotate(5deg); }
        }
        .zepra-logo {
            width: 140px;
            height: 140px;
            position: relative;
            animation: float 6s ease-in-out infinite;
        }
        .zepra-logo svg {
            width: 100%;
            height: 100%;
            filter: drop-shadow(0 8px 24px rgba(255,255,255,0.4));
        }
        .sparkle {
            position: absolute;
            width: 6px;
            height: 6px;
            background: white;
            border-radius: 50%;
            animation: sparkle 2s ease-in-out infinite;
        }
        @keyframes sparkle {
            0%, 100% { opacity: 0; transform: scale(0); }
            50% { opacity: 1; transform: scale(1); }
        }
        .sparkle:nth-child(1) { top: 10%; left: 15%; animation-delay: 0s; }
        .sparkle:nth-child(2) { top: 20%; right: 10%; animation-delay: 0.5s; }
        .sparkle:nth-child(3) { bottom: 25%; left: 20%; animation-delay: 1s; }
        .sparkle:nth-child(4) { bottom: 15%; right: 15%; animation-delay: 1.5s; }
        
        .search-container {
            width: 90%;
            max-width: 680px;
            background: rgba(255,255,255,0.98);
            border-radius: 50px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.12);
            padding: 6px 16px;
            display: flex;
            align-items: center;
            gap: 12px;
            animation: fadeInUp 0.8s ease-out 0.2s backwards;
            transition: all 0.3s ease;
        }
        .search-container:hover {
            box-shadow: 0 12px 48px rgba(0,0,0,0.16);
            transform: translateY(-2px);
        }
        .search-icons {
            display: flex;
            gap: 6px;
            padding-left: 4px;
        }
        .icon-btn {
            width: 40px;
            height: 40px;
            border: none;
            background: transparent;
            border-radius: 10px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.2s ease;
        }
        .icon-btn:hover {
            background: rgba(0,0,0,0.06);
            transform: scale(1.05);
        }
        .icon-btn svg { width: 20px; height: 20px; fill: #666; }
        .search-input {
            flex: 1;
            border: none;
            font-size: 15px;
            padding: 16px 12px;
            outline: none;
            background: transparent;
            color: #1a1a1a;
        }
        .search-input::placeholder { color: #666; }
        .action-icons {
            display: flex;
            gap: 6px;
            padding-right: 4px;
        }
        .voice-btn, .ai-btn {
            width: 44px;
            height: 44px;
            border: none;
            background: transparent;
            border-radius: 50%;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.2s ease;
        }
        .voice-btn:hover, .ai-btn:hover {
            background: rgba(0,0,0,0.06);
            transform: scale(1.08);
        }
        .voice-btn svg, .ai-btn svg { width: 22px; height: 22px; fill: #333; }
    </style>
</head>
<body>
    <div class="logo-container">
        <div class="zepra-logo">
            <div class="sparkle"></div>
            <div class="sparkle"></div>
            <div class="sparkle"></div>
            <div class="sparkle"></div>
            <svg viewBox="0 0 120 120" fill="none">
                <g transform="translate(20, 15)">
                    <path d="M40 10 Q45 5, 50 10 L55 30 L70 28 Q75 28, 73 33 L60 50 L68 70 Q70 75, 65 73 L50 60 L35 73 Q30 75, 32 70 L40 50 L27 33 Q25 28, 30 28 L45 30 Z" fill="white" stroke="white" stroke-width="2.5"/>
                    <circle cx="15" cy="8" r="3.5" fill="white" opacity="0.8"/>
                    <circle cx="70" cy="15" r="2.5" fill="white" opacity="0.7"/>
                    <circle cx="78" cy="45" r="3" fill="white" opacity="0.75"/>
                </g>
            </svg>
        </div>
    </div>
    <div class="search-container">
        <div class="search-icons">
            <button class="icon-btn" title="Text">
                <svg viewBox="0 0 24 24"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zM6 20V4h7v5h5v11H6z"/></svg>
            </button>
            <button class="icon-btn" title="Image">
                <svg viewBox="0 0 24 24"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z"/></svg>
            </button>
        </div>
        <input class="search-input" type="text" placeholder="Let's give your dream life. What you create today?.." autofocus>
        <div class="action-icons">
            <button class="voice-btn" title="Voice">
                <svg viewBox="0 0 24 24"><path d="M12 14c1.66 0 3-1.34 3-3V5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3z"/><path d="M17 11c0 2.76-2.24 5-5 5s-5-2.24-5-5H5c0 3.53 2.61 6.43 6 6.92V21h2v-3.08c3.39-.49 6-3.39 6-6.92h-2z"/></svg>
            </button>
            <button class="ai-btn" title="AI">
                <svg viewBox="0 0 24 24"><path d="M12 1c-4.97 0-9 4.03-9 9v7c0 1.66 1.34 3 3 3h3v-8H5v-2c0-3.87 3.13-7 7-7s7 3.13 7 7v2h-4v8h3c1.66 0 3-1.34 3-3v-7c0-4.97-4.03-9-9-9z"/></svg>
            </button>
        </div>
    </div>
</body>
</html>
)HTML");
        } else if (request.url() == "zepra://settings") {
            response.setBody(R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Settings - Zepra Browser</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1e2029;
            color: #e8eaed;
            min-height: 100vh;
            display: flex;
        }
        
        /* Sidebar Navigation */
        .sidebar {
            width: 220px;
            background: #202124;
            padding: 20px 0;
            min-height: 100vh;
            border-right: 1px solid #3c4043;
        }
        .sidebar-title {
            color: #8ab4f8;
            font-size: 18px;
            font-weight: 600;
            padding: 0 20px 20px;
            border-bottom: 1px solid #3c4043;
            margin-bottom: 10px;
        }
        .sidebar-item {
            padding: 14px 20px;
            cursor: pointer;
            color: #9aa0a6;
            transition: all 0.2s;
            display: flex;
            align-items: center;
            gap: 12px;
        }
        .sidebar-item:hover { background: #3c4043; }
        .sidebar-item.active {
            background: #8ab4f8;
            color: #202124;
        }
        .sidebar-item .icon {
            width: 20px;
            height: 20px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        /* Content Area */
        .content {
            flex: 1;
            padding: 40px;
            overflow-y: auto;
        }
        .page { display: none; }
        .page.active { display: block; }
        .page-title {
            font-size: 28px;
            font-weight: 600;
            color: #e8eaed;
            margin-bottom: 30px;
        }
        
        /* Settings Cards */
        .card {
            background: #292a2d;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid #3c4043;
        }
        .card h3 {
            color: #e8eaed;
            font-size: 16px;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid #3c4043;
        }
        .setting-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 16px 0;
            border-bottom: 1px solid #3c404380;
        }
        .setting-row:last-child { border-bottom: none; }
        .setting-info { flex: 1; }
        .setting-label {
            color: #e8eaed;
            font-size: 14px;
            font-weight: 500;
            margin-bottom: 4px;
        }
        .setting-desc {
            color: #9aa0a6;
            font-size: 12px;
        }
        
        /* Toggle Switch */
        .toggle {
            width: 44px;
            height: 24px;
            background: #5f6368;
            border-radius: 12px;
            position: relative;
            cursor: pointer;
            transition: background 0.3s;
        }
        .toggle.active { background: #8ab4f8; }
        .toggle::after {
            content: '';
            position: absolute;
            width: 20px;
            height: 20px;
            background: white;
            border-radius: 50%;
            top: 2px;
            left: 2px;
            transition: left 0.3s;
        }
        .toggle.active::after { left: 22px; }
        
        /* Form Controls */
        select, input[type="text"], input[type="number"] {
            background: #3c4043;
            color: #e8eaed;
            border: 1px solid #5f6368;
            border-radius: 6px;
            padding: 10px 14px;
            font-size: 14px;
            min-width: 200px;
        }
        select:focus, input:focus {
            outline: none;
            border-color: #8ab4f8;
        }
        
        /* Buttons */
        .btn {
            padding: 10px 20px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
            border: none;
        }
        .btn-primary {
            background: #8ab4f8;
            color: #202124;
        }
        .btn-primary:hover { background: #aecbfa; }
        .btn-danger {
            background: #ea4335;
            color: white;
        }
        .btn-danger:hover { background: #f44336; }
        .btn-secondary {
            background: #3c4043;
            color: #e8eaed;
        }
        .btn-secondary:hover { background: #5f6368; }
        
        /* Slider */
        input[type="range"] {
            width: 200px;
            accent-color: #8ab4f8;
        }
        
        /* About Page */
        .about-logo {
            width: 80px;
            height: 80px;
            background: linear-gradient(135deg, #8ab4f8, #667eea);
            border-radius: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 32px;
            font-weight: bold;
            color: white;
            margin-bottom: 20px;
        }
        .version {
            color: #9aa0a6;
            margin-bottom: 20px;
        }
        .link-item {
            color: #8ab4f8;
            text-decoration: none;
            display: block;
            padding: 8px 0;
        }
        .link-item:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="sidebar">
        <div class="sidebar-title">Settings</div>
        <div class="sidebar-item active" data-page="general">
            <span class="icon">G</span>General
        </div>
        <div class="sidebar-item" data-page="appearance">
            <span class="icon">A</span>Appearance
        </div>
        <div class="sidebar-item" data-page="privacy">
            <span class="icon">P</span>Privacy
        </div>
        <div class="sidebar-item" data-page="search">
            <span class="icon">S</span>Search
        </div>
        <div class="sidebar-item" data-page="advanced">
            <span class="icon">X</span>Advanced
        </div>
        <div class="sidebar-item" data-page="about">
            <span class="icon">i</span>About
        </div>
    </div>
    
    <div class="content">
        <!-- General Page -->
        <div class="page active" id="general">
            <h1 class="page-title">General</h1>
            <div class="card">
                <h3>Startup</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">On startup</div>
                        <div class="setting-desc">Choose what to show when Zepra starts</div>
                    </div>
                    <select>
                        <option>Open Ketivee homepage</option>
                        <option>Continue where you left off</option>
                        <option>Open new tab</option>
                    </select>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Homepage URL</div>
                        <div class="setting-desc">Default page when clicking Home</div>
                    </div>
                    <input type="text" value="https://ketivee.com" />
                </div>
            </div>
            <div class="card">
                <h3>Downloads</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Download location</div>
                        <div class="setting-desc">Where files are saved</div>
                    </div>
                    <input type="text" value="~/Downloads" />
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Ask where to save</div>
                        <div class="setting-desc">Prompt before each download</div>
                    </div>
                    <div class="toggle"></div>
                </div>
            </div>
            <div class="card">
                <h3>Updates</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Auto-update</div>
                        <div class="setting-desc">Automatically update Zepra Browser</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
        </div>
        
        <!-- Appearance Page -->
        <div class="page" id="appearance">
            <h1 class="page-title">Appearance</h1>
            <div class="card">
                <h3>Theme</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Color theme</div>
                        <div class="setting-desc">Choose your preferred theme</div>
                    </div>
                    <select>
                        <option>Dark</option>
                        <option>Light</option>
                        <option>System</option>
                    </select>
                </div>
            </div>
            <div class="card">
                <h3>Font</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Font size</div>
                        <div class="setting-desc">Default text size: <span id="fontSizeValue">14px</span></div>
                    </div>
                    <input type="range" min="10" max="24" value="14" id="fontSizeSlider" />
                </div>
            </div>
            <div class="card">
                <h3>Zoom</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Page zoom</div>
                        <div class="setting-desc">Default zoom level: <span id="zoomValue">100%</span></div>
                    </div>
                    <input type="range" min="50" max="200" value="100" id="zoomSlider" />
                </div>
            </div>
            <div class="card">
                <h3>Toolbar</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Show bookmarks bar</div>
                        <div class="setting-desc">Display bookmarks below address bar</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Show home button</div>
                        <div class="setting-desc">Display home button in toolbar</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
        </div>
        
        <!-- Privacy Page -->
        <div class="page" id="privacy">
            <h1 class="page-title">Privacy and Security</h1>
            <div class="card">
                <h3>Browsing Data</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Save browsing history</div>
                        <div class="setting-desc">Remember visited pages</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Accept cookies</div>
                        <div class="setting-desc">Allow websites to store cookies</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Save passwords</div>
                        <div class="setting-desc">Offer to save login credentials</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
            <div class="card">
                <h3>Security</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Block third-party cookies</div>
                        <div class="setting-desc">Prevent cross-site tracking</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Send Do Not Track</div>
                        <div class="setting-desc">Request websites not to track you</div>
                    </div>
                    <div class="toggle"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">HTTPS-only mode</div>
                        <div class="setting-desc">Always use secure connections</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
            <div class="card">
                <h3>Clear Data</h3>
                <div class="setting-row">
                    <button class="btn btn-danger">Clear Cookies</button>
                    <button class="btn btn-danger">Clear Cache</button>
                    <button class="btn btn-danger">Clear History</button>
                </div>
            </div>
        </div>
        
        <!-- Search Page -->
        <div class="page" id="search">
            <h1 class="page-title">Search</h1>
            <div class="card">
                <h3>Default Search Engine</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Search engine</div>
                        <div class="setting-desc">Used for address bar searches</div>
                    </div>
                    <select>
                        <option>Ketivee Search</option>
                        <option>Google</option>
                        <option>DuckDuckGo</option>
                        <option>Bing</option>
                        <option>Custom</option>
                    </select>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Custom search URL</div>
                        <div class="setting-desc">Use %s for search query</div>
                    </div>
                    <input type="text" value="https://ketivee.com/search?q=%s" />
                </div>
            </div>
            <div class="card">
                <h3>Search Features</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Show search suggestions</div>
                        <div class="setting-desc">Display suggestions as you type</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Predictive search</div>
                        <div class="setting-desc">AI-powered search predictions</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
        </div>
        
        <!-- Advanced Page -->
        <div class="page" id="advanced">
            <h1 class="page-title">Advanced</h1>
            <div class="card">
                <h3>Performance</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Hardware acceleration</div>
                        <div class="setting-desc">Use GPU for rendering when available</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Maximum cache size</div>
                        <div class="setting-desc">Disk space for cached content</div>
                    </div>
                    <input type="number" value="500" min="50" max="10000" /> MB
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Max connections per server</div>
                        <div class="setting-desc">Parallel download connections</div>
                    </div>
                    <input type="number" value="6" min="1" max="20" />
                </div>
            </div>
            <div class="card">
                <h3>Content</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Enable JavaScript</div>
                        <div class="setting-desc">Run ZepraScript on web pages</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Autoplay media</div>
                        <div class="setting-desc">Allow videos to play automatically</div>
                    </div>
                    <div class="toggle"></div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Allow notifications</div>
                        <div class="setting-desc">Let websites show notifications</div>
                    </div>
                    <div class="toggle active"></div>
                </div>
            </div>
            <div class="card">
                <h3>Reset</h3>
                <div class="setting-row">
                    <button class="btn btn-danger">Reset All Settings</button>
                    <button class="btn btn-secondary">Export Settings</button>
                    <button class="btn btn-secondary">Import Settings</button>
                </div>
            </div>
        </div>
        
        <!-- About Page -->
        <div class="page" id="about">
            <h1 class="page-title">About Zepra Browser</h1>
            <div class="card">
                <div class="about-logo">Z</div>
                <h2 style="margin-bottom: 8px;">Zepra Browser</h2>
                <div class="version">Version 1.0.0</div>
                <p style="color: #9aa0a6; line-height: 1.6; margin-bottom: 20px;">
                    A fast, secure, and privacy-focused web browser built with ZepraEngine and ZepraScript.
                </p>
            </div>
            <div class="card">
                <h3>Engine Information</h3>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Rendering Engine</div>
                        <div class="setting-desc">ZepraEngine 1.0</div>
                    </div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">JavaScript Engine</div>
                        <div class="setting-desc">ZepraScript 1.0</div>
                    </div>
                </div>
                <div class="setting-row">
                    <div class="setting-info">
                        <div class="setting-label">Developer Tools</div>
                        <div class="setting-desc">ZepraDevTools (Press F12)</div>
                    </div>
                </div>
            </div>
            <div class="card">
                <h3>Links</h3>
                <a href="https://ketivee.com" class="link-item">Ketivee Homepage</a>
                <a href="https://ketivee.com/help" class="link-item">Help Center</a>
                <a href="https://ketivee.com/privacy" class="link-item">Privacy Policy</a>
                <a href="https://ketivee.com/terms" class="link-item">Terms of Service</a>
            </div>
        </div>
    </div>
    
    <script>
        // Sidebar navigation
        document.querySelectorAll('.sidebar-item').forEach(item => {
            item.addEventListener('click', () => {
                document.querySelectorAll('.sidebar-item').forEach(i => i.classList.remove('active'));
                document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
                item.classList.add('active');
                document.getElementById(item.dataset.page).classList.add('active');
            });
        });
        
        // Toggle switches
        document.querySelectorAll('.toggle').forEach(toggle => {
            toggle.addEventListener('click', () => {
                toggle.classList.toggle('active');
            });
        });
        
        // Sliders
        document.getElementById('fontSizeSlider').addEventListener('input', function() {
            document.getElementById('fontSizeValue').textContent = this.value + 'px';
        });
        document.getElementById('zoomSlider').addEventListener('input', function() {
            document.getElementById('zoomValue').textContent = this.value + '%';
        });
    </script>
</body>
</html>
)HTML");
        } else {
             return Response(404, "Not Found");
        }
        return response;
    }
    
    // Use Networking Loader
    if (networkLoader_ && networkLoader_->isSupportedScheme(request.url())) {
        Networking::ResourceResponse netResp = networkLoader_->loadUrl(request.url());
        
        if (netResp.success) {
            Response response(netResp.statusCode);
            response.setUrl(request.url());
            response.setBody(netResp.data);
            if (!netResp.contentType.empty()) {
                response.headers().set("Content-Type", netResp.contentType);
            }
            
            // Cache if enabled
            if (cacheEnabled_ && response.ok() && request.method() == HttpMethod::GET) {
                cache_[request.url()] = response;
            }
            return response;
        } else {
             // Network error
             return Response(0, "Network Error: " + netResp.error);
        }
    }
    
    return Response(400, "Unsupported Scheme");
}

void ResourceLoader::loadAsync(const Request& request, const ResourceCallbacks& callbacks) {
    // For now, just call sync version
    // Real implementation would use async I/O
    Response response = load(request);
    
    if (response.ok()) {
        if (callbacks.onComplete) callbacks.onComplete(response);
    } else {
        if (callbacks.onError) callbacks.onError("Request failed: " + std::to_string(response.status()));
    }
}

Response ResourceLoader::loadURL(const std::string& url) {
    Request request(url);
    return load(request);
}

void ResourceLoader::loadURLAsync(const std::string& url,
                                  std::function<void(const Response&)> onComplete,
                                  std::function<void(const std::string&)> onError) {
    ResourceCallbacks callbacks;
    callbacks.onComplete = std::move(onComplete);
    callbacks.onError = std::move(onError);
    
    Request request(url);
    loadAsync(request, callbacks);
}

void ResourceLoader::cancel(const std::string& url) {
    (void)url;
    // Would cancel pending request
}

void ResourceLoader::cancelAll() {
    // Would cancel all pending requests
}

void ResourceLoader::clearCache() {
    cache_.clear();
}

// =============================================================================
// URL
// =============================================================================

URL::URL(const std::string& url) {
    parse(url);
}

URL::URL(const std::string& url, const std::string& base) {
    std::string resolved = resolve(base, url);
    parse(resolved);
}

void URL::parse(const std::string& url) {
    href_ = url;
    
    // Simple URL parsing
    size_t pos = 0;
    
    // Protocol
    size_t protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        protocol_ = url.substr(0, protocolEnd + 1);
        pos = protocolEnd + 3;
    } else {
        valid_ = false;
        return;
    }
    
    // Find path start
    size_t pathStart = url.find('/', pos);
    if (pathStart == std::string::npos) pathStart = url.length();
    
    // Host (including port)
    host_ = url.substr(pos, pathStart - pos);
    
    // Split hostname and port
    size_t portPos = host_.find(':');
    if (portPos != std::string::npos) {
        hostname_ = host_.substr(0, portPos);
        port_ = std::stoi(host_.substr(portPos + 1));
    } else {
        hostname_ = host_;
        if (protocol_ == "http:") port_ = 80;
        else if (protocol_ == "https:") port_ = 443;
    }
    
    if (pathStart < url.length()) {
        // Find query string
        size_t queryPos = url.find('?', pathStart);
        size_t hashPos = url.find('#', pathStart);
        
        if (queryPos != std::string::npos) {
            pathname_ = url.substr(pathStart, queryPos - pathStart);
            if (hashPos != std::string::npos) {
                search_ = url.substr(queryPos, hashPos - queryPos);
                hash_ = url.substr(hashPos);
            } else {
                search_ = url.substr(queryPos);
            }
        } else if (hashPos != std::string::npos) {
            pathname_ = url.substr(pathStart, hashPos - pathStart);
            hash_ = url.substr(hashPos);
        } else {
            pathname_ = url.substr(pathStart);
        }
    } else {
        pathname_ = "/";
    }
    
    valid_ = true;
}

std::string URL::resolve(const std::string& base, const std::string& relative) {
    // Absolute URL
    if (relative.find("://") != std::string::npos) {
        return relative;
    }
    
    URL baseUrl(base);
    if (!baseUrl.isValid()) return relative;
    
    // Protocol-relative
    if (relative.length() >= 2 && relative[0] == '/' && relative[1] == '/') {
        return baseUrl.protocol() + relative;
    }
    
    // Absolute path
    if (!relative.empty() && relative[0] == '/') {
        return baseUrl.origin() + relative;
    }
    
    // Relative path
    std::string basePath = baseUrl.pathname();
    size_t lastSlash = basePath.rfind('/');
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    }
    
    return baseUrl.origin() + basePath + relative;
}

} // namespace Zepra::WebCore
