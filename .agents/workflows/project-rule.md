---
description: How to edit code and mentain a healthy devlopment and make stable to our browser
---

# Zepra Browser — Agent Development Rules

## Core Mandate
You are editing a production C++ browser engine (ZepraScript + NXRender pipeline).
Every edit must leave the codebase in a **compilable, stable state**.

## Before Editing
- Read the relevant header (.h) before touching any .cpp file
- Check existing CMakeLists.txt targets before adding/moving files
- Never assume a symbol exists — search for its declaration first
- If unsure about a subsystem's API, read its SKILL/doc file first

## Code Editing Rules
- **No new warnings tolerated** — treat all warnings as errors (already set in CMake)
- Match existing code style exactly (indentation, brace style, naming convention)
- Never remove `#include` guards or change header include order without reason
- Do not introduce V8, SpiderMonkey, WebKit, or Chromium namespace names
- All new files must carry the KPL-2.0 license header
- Prefer forward declarations over new includes in headers

## Build Safety
- After any structural change, mentally verify CMakeLists.txt is still consistent
- Never delete a file — mark it for review instead
- Use `git mv` only for renames/moves, never manual copy+delete
- Do not run build commands unless explicitly asked

## Stability Rules
- DOM/layout/render pipeline changes require a corresponding test note
- GC subsystem is sealed — do not modify heap or GC files without explicit instruction
- ZepraScript VM stress test must still pass conceptually after any JS engine edit
- Networking layer (NXHttp, NXCrypto, curl engine) changes need SSL safety review note

## When You're Unsure
- Stop and ask rather than guess
- Output a diff-style summary of what you plan to change before doing it
- Flag any change that touches more than 6 to 8 files at once for human review