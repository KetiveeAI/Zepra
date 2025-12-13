# ZepraBrowser Development TODO

## Priority 1: Core Web Engine

### Networking Layer
- [ ] HTTP/1.1 client with curl integration
- [ ] HTTPS/TLS support with OpenSSL
- [ ] HTTP/2 support
- [ ] DNS resolution and caching
- [ ] Cookie management (storage, expiry, secure flags)
- [ ] CORS implementation
- [ ] Redirect handling (301, 302, 307, 308)
- [ ] Request/response streaming
- [ ] WebSocket client

### HTML Parser
- [ ] Tokenizer (HTML5 spec compliant)
- [ ] Tree builder (DOM construction)
- [ ] Error recovery and quirks mode
- [ ] Entity decoding
- [ ] Script/style element handling
- [ ] Template element support
- [ ] Custom elements

### CSS Engine
- [ ] CSS tokenizer and parser
- [ ] Selector matching (specificity)
- [ ] Cascade and inheritance
- [ ] Box model calculation
- [ ] Flexbox layout
- [ ] Grid layout
- [ ] Positioned elements (absolute, relative, fixed, sticky)
- [ ] Float and clear
- [ ] Media queries
- [ ] CSS variables (custom properties)
- [ ] Animations and transitions

### DOM Implementation
- [ ] Node, Element, Document interfaces
- [ ] Event system (capture, bubble, prevent)
- [ ] MutationObserver
- [ ] querySelector/querySelectorAll
- [ ] classList, dataset
- [ ] innerHTML, textContent
- [ ] createElement, appendChild, removeChild
- [ ] Attribute manipulation

---

## Priority 2: ZepraScript Integration

### JavaScript Engine (ZepraScript)
- [ ] ES2024 compliance
- [ ] JIT compilation tiers
- [ ] Garbage collection
- [ ] async/await, Promises
- [ ] Modules (import/export)
- [ ] Classes and inheritance
- [ ] Proxies and Reflect
- [ ] WeakMap, WeakSet
- [ ] BigInt, Symbol
- [ ] RegExp (full spec)

### Web APIs (via ZepraScript)
- [ ] Console API
- [ ] Fetch API
- [ ] DOM manipulation APIs
- [ ] setTimeout, setInterval, requestAnimationFrame
- [ ] LocalStorage, SessionStorage
- [ ] IndexedDB
- [ ] Web Workers
- [ ] Service Workers
- [ ] History API
- [ ] Geolocation API
- [ ] Notifications API
- [ ] Clipboard API
- [ ] Canvas 2D API
- [ ] WebGL API
- [ ] WebAudio API
- [ ] MediaDevices API

---

## Priority 3: Rendering Engine

### NXRENDER Graphics
- [ ] Real OpenGL context (not stub)
- [ ] Texture atlas for images
- [ ] Font rendering (FreeType + HarfBuzz)
- [ ] Text layout (line breaking, bidi)
- [ ] Image decoding (PNG, JPEG, WebP, GIF, SVG)
- [ ] GPU-accelerated compositing
- [ ] Damage tracking
- [ ] VSync and frame pacing

### Layout Engine
- [ ] Block formatting context
- [ ] Inline formatting context
- [ ] Table layout
- [ ] Multi-column layout
- [ ] Flow layout
- [ ] Reflow optimization
- [ ] Layer tree

### Paint and Composite
- [ ] Paint order (z-index, stacking context)
- [ ] Opacity and blending
- [ ] Filters (blur, drop-shadow)
- [ ] Transforms (2D and 3D)
- [ ] Clipping and masking
- [ ] Border rendering (solid, dashed, radius)
- [ ] Box shadows
- [ ] Gradients (linear, radial, conic)

---

## Priority 4: Browser Features

### Tab Management  
- [ ] Tab creation/destruction
- [ ] Tab switching
- [ ] Tab reordering (drag)
- [ ] Tab groups
- [ ] Tab hibernation (memory)
- [ ] Private/incognito tabs

### Navigation
- [ ] URL parsing (RFC 3986)
- [ ] Back/forward history
- [ ] Address bar with autocomplete
- [ ] Search engine integration
- [ ] Bookmarks
- [ ] Download manager

### DevTools
- [ ] Elements panel (DOM inspector)
- [ ] Console panel
- [ ] Network panel
- [ ] Sources panel (debugger)
- [ ] Performance panel
- [ ] Application panel (storage)
- [ ] WebDriver protocol (Selenium)

### Settings
- [ ] Privacy settings
- [ ] Security settings
- [ ] Appearance (themes)
- [ ] Default search engine
- [ ] Language preferences
- [ ] Permissions manager

---

## Priority 5: Security

### Content Security
- [ ] Same-origin policy
- [ ] Content Security Policy (CSP)
- [ ] X-Frame-Options
- [ ] XSS protection
- [ ] HTTPS-only mode
- [ ] Certificate verification
- [ ] Safe browsing

### Process Isolation
- [ ] Sandbox per tab
- [ ] IPC between processes
- [ ] Memory protection
- [ ] File system sandboxing

### Authentication
- [ ] Password manager
- [ ] Autofill
- [ ] WebAuthn / FIDO2
- [ ] Two-factor authentication

---

## Priority 6: Platform Integration

### System Integration
- [ ] Default browser registration
- [ ] File associations
- [ ] Protocol handlers
- [ ] Notifications
- [ ] System tray
- [ ] Auto-updates

### Accessibility
- [ ] Screen reader support
- [ ] Keyboard navigation
- [ ] High contrast mode
- [ ] Font scaling
- [ ] Reduced motion

---

## Immediate Next Steps

1. **Week 1**: HTTP client with curl, basic HTML tokenizer
2. **Week 2**: DOM tree construction, CSS tokenizer
3. **Week 3**: Box model, block/inline layout
4. **Week 4**: ZepraScript DOM bindings
5. **Week 5**: Text rendering with FreeType
6. **Week 6**: Image loading and rendering
7. **Week 7**: Event system (mouse, keyboard)
8. **Week 8**: Navigation and history
