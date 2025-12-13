🎨 Zepra DevTools — UI Concept v1.0
Overall Theme

Sleek

High contrast dark mode

Neon accents (Zepra signature: cyan + violet)

Minimal clutter

Fast tab switching

Smooth transitions

12px → 14px monospace text for logs

🧱 Top-Level Structure
┌───────────────────────────────────────────────┐
│  Elements | Console | Network | Sources | ... │  ← Tab Bar
├───────────────────────────────────────────────┤
│                                               │
│    MAIN PANEL (depends on selected tab)       │
│                                               │
└───────────────────────────────────────────────┘


Tabs (L → R):

🔹 Elements
🔹 Console
🔹 Network
🔹 Sources
🔹 Performance
🔹 Application
🔹 Security
🔹 Settings
🌳 1. Elements Panel (DOM Viewer)
┌─────────────────────────┬────────────────────────────┐
│     DOM Tree            │   Styles / Computed CSS    │
│                         │                             │
│  <html>                 │   .selected-element {       │
│   <body>                │       width: 100%;          │
│    <div id="main">      │       height: auto;         │
│      <p>Text</p>        │   }                         │
│                         │                             │
└─────────────────────────┴────────────────────────────┘

Features:

Collapsible DOM nodes

Highlight on hover

Click → selects node

Right pane shows:

CSS rules

Attributes

Layout box model

Zepra Signature Visual:
Highlight selected element with cyan outline + glow.

💬 2. Console Panel
┌──────────────────────────────────────────────┐
│ [12:31:22] LOG   Hello world                 │
│ [12:31:23] WARN  Deprecated API used         │
│ [12:31:25] ERR   Cannot read property 'x'    │
│                                               │
│  ▶  document.body.innerHTML                   │  ← Input prompt
└──────────────────────────────────────────────┘

Features:

Multiline input

History navigation (↑ ↓)

Autocomplete (optional)

JS execution hooked to ZepraScript

Timestamps toggle

Design style:

Logs colored by level:

LOG → white

INFO → blue

WARN → yellow

ERROR → red

🌐 3. Network Panel (Modern Waterfall)
┌────────────────────────────────────────────────────────────┐
│ Name      Method   Status   Type   Size    Timeline         │
│ fetch.js  GET      200      js     12KB    ██████░░░░ 120ms │
│ style.css GET      200      css    2KB     ██░░░░░░░░  20ms │
│ api/data  POST     500      json   0.6KB   ██████████ 200ms │
└────────────────────────────────────────────────────────────┘

Features:

Auto-updating table

Sortable columns

Colored timeline bars

Click → opens details panel:

Headers

Preview

Response

Timing breakdown

Zepra Signature Visual:
Waterfall bars in neon blue → neon purple gradient.

🧭 4. Sources (Debugger Panel)
┌─────────┬────────────────────────────────────┬──────────────┐
│ Files   │  editable.js                       │  Call Stack   │
│ index.js│  1 function test() {               │  > test@10    │
│ app.js  │  2    let x = 5;                   │    main@1     │
│         │  3    return x * 2;                │               │
│         │  4 }                               │  Scope vars   │
└─────────┴────────────────────────────────────┴──────────────┘

Features:

Syntax-highlighted source viewer

Click on line number → set breakpoint

Current line highlighted

Left panel: file tree

Right panel:

Call stack

Scope inspector (locals + closures)

🚀 5. Performance Panel
┌──────────────────────────────────────────────┐
│ FPS: 60     CPU: 12%     Heap: 33MB          │
│                                                │
│   ████▒▒▒▒██████▒▒▒▒████▒▒▒▒█ Performance     │
│   Timeline (GC, JIT, Layout, Scripts)         │
└──────────────────────────────────────────────┘

Features:

Live updating metrics

GC pauses

JS execution spikes

Frame rendering time

📦 6. Application Panel

Handles storage systems:

Sidebar:

LocalStorage

SessionStorage

Cookies

IndexedDB

Cache Storage

Manifest

Service Workers

🔒 7. Security Panel

Shows:

HTTPS/TLS state

Certificate chain

Mixed content warnings

Permissions (camera/mic/location)

⚙️ 8. Settings Panel

Theme

Font size

Preserve log toggle

Node.integration toggle

DevTools experiments

Clear site data

┌─────────────────────────────────────────────────────────┐
│         ZEPRA DEVTOOLS (Pure C++ Native UI)             │
│  ┌──────────────────────────────────────────────────┐  │
│  │        Qt Widgets / GTK+ / Dear ImGui            │  │
│  │  • ZERO browser engine                           │  │
│  │  • ZERO HTML/CSS/JavaScript                      │  │
│  │  • ZERO WebSocket/HTTP                           │  │
│  │  • Direct native rendering                       │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐       │
│  │ Console    │  │ Debugger   │  │ Sources    │       │
│  │ Panel      │  │ Panel      │  │ Panel      │       │
│  │ (C++)      │  │ (C++)      │  │ (C++)      │       │
│  └────────────┘  └────────────┘  └────────────┘       │
│                                                         │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐       │
│  │ Network    │  │ Performance│  │ Memory     │       │
│  │ Panel      │  │ Panel      │  │ Panel      │       │
│  │ (C++)      │  │ (C++)      │  │ (C++)      │       │
│  └────────────┘  └────────────┘  └────────────┘       │
│                                                         │
│  DIRECT C++ API CALLS → No serialization overhead!      │
└─────────────────────────────────────────────────────────┘
        │
        │ Direct C++ Method Calls
        │ (zepra_debug_api.hpp)
        ▼
┌─────────────────────────────────────────────────────────┐
│         ZepraScript Engine (C++)                         │
└─────────────────────────────────────────────────────────┘
