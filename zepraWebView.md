🔧 ZEPRA WEB VIEW — DEVTOOLS NAMING SYSTEM

Internal name: Zepra Web View
Public-facing tool: Zepra DevView

🧭 TOP-LEVEL PANELS (Primary Tabs)
1️⃣ Structure

(formerly Elements)

Purpose: DOM, layout, accessibility, styles

Includes:

DOM Tree

CSS Rules

Computed Styles

Layout Metrics

Accessibility Tree

Shadow DOM

📌 Why “Structure”?
Because it shows how the page is built, not just “elements”.

2️⃣ Scripts

(formerly Sources)

Purpose: JavaScript + execution control

Includes:

Script Explorer

Breakpoints (line, event, symbolic)

Call Stack

Scope Viewer

Blackboxing

Source Maps

WASM Viewer

📌 Why “Scripts”?
Clear, language-agnostic, future-proof (JS, WASM, others).

3️⃣ Console

(kept — everyone knows this)

Purpose: Live execution & logging

Includes:

Runtime Console

Command Line API

Snippets

Console Filters

Errors & Warnings

Execution Context Selector

📌 Keep it boring. Developers expect it.

4️⃣ Network

(kept — but enhanced)

Purpose: Transport & data flow

Includes:

Requests Timeline

Headers

Payload

Cookies

WebSocket Frames

Streaming Media

Cache / Service Worker routing

📌 Network is sacred — don’t rename.

5️⃣ Render

(formerly Layers + Page Overlay)

Purpose: Visual pipeline & GPU

Includes:

Layer Tree

Paint Records

Compositing

Overdraw

FPS Meter

GPU Memory

Viewport Overlays

📌 “Render” maps directly to NXRender.

6️⃣ Timeline

(formerly Timelines / Performance)

Purpose: Performance profiling

Includes:

CPU Timeline

Script Execution

Layout / Paint

Media Decode

Memory Allocation

GC Events

Frame Budget

📌 Engineers think in timelines, not “performance tabs”.

7️⃣ Storage

(formerly Application)

Purpose: Origin-scoped data

Includes:

localStorage

sessionStorage

IndexedDB

Cache Storage

Cookies

Permissions

Storage Quotas

📌 “Application” is vague. Storage is explicit.

8️⃣ Media

(split out — your advantage)

Purpose: Audio / Video / Streaming

Includes:

Media Sessions

Audio Graph (Web Audio)

Video Decode

WebCodecs

Stream Latency

DRM / EME state

Cloud Gaming Metrics

📌 WebKit hides this. You own it.

9️⃣ Security

(explicit, not hidden)

Purpose: Trust & isolation

Includes:

Origin Isolation

CSP Violations

Permissions State

Secure Context

Certificate Info

Mixed Content

Trusted Types

📌 This makes Zepra feel enterprise-grade.

🔟 Audit

(kept, but expanded)

Purpose: Automated checks

Includes:

Accessibility Audit

Performance Audit

Security Audit

API Usage Audit

Compatibility Check

📌 Later: AI-assisted suggestions.

⚙️ SECONDARY TOOLS (Side / Modal)
🔹 Inspector Core

(formerly Inspector Bootstrap Script)

Internal engine glue. Not exposed by default.

🔹 Overrides

(formerly Local Overrides)

Local file mapping

Header overrides

Request rewriting

🔹 Device

(formerly Device Settings)

Viewport simulation

Input simulation

Network throttling

Sensor emulation

🔹 Search

Global search across:

DOM

CSS

Scripts

Network

⌨️ KEYBOARD SHORTCUTS (Zepra-style)

Ctrl + Shift + Z → Open Zepra Web View

Ctrl + P → Script search

Ctrl + Shift + F → Global search

Ctrl + Shift + C → Inspect element

Ctrl + Shift + M → Device mode

🧠 Naming philosophy (important)

You avoided:

WebKit legacy terms

Chrome clones

Confusing metaphors

You chose:

System-oriented names

Engine-aligned terms

Future-proof language

This matters when:

you add AI tools

you add WASM debugging

you add OS-level integration