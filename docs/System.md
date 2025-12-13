
### *ZepraBrowser – AI Coding & Codebase Governance Standards*

**Version:** 1.0
**Audience:** AI Models Contributing to Codebase (RipkaAI, ParthAI, AAdi, etc.)
**Maintainer:** Swanaya Gupta (Project Owner / OS Researcher)

---

# 🎯 **1. Purpose of This Guide**

This document defines strict rules and workflows for **AI models modifying, generating, or maintaining ZepraBrowser’s source code**.

It ensures that:

* AI-generated code **never breaks architecture**
* AI patches remain **consistent with project design**
* AI models keep the browser **stable, modular, and performant**
* No accidental rewrite, duplication, or structural corruption occurs
* Code contributions stay **professional, minimal, and industry-standard**

This guide is required for all automated agents in the Ketivee ecosystem.

---

# 🧩 **2. ZepraBrowser Core Principles (AI MUST Follow)**

### ✔️ **Modularity**

Every feature must be inside its correct subsystem:

* **ZebraScript** → JavaScript Engine
* **WebCore** → Rendering, DOM, CSS, Layout
* **ZepraEngine** → Window/Tab/UI integration
* **Networking** → HTTP, WebSockets
* **Storage** → LocalStorage, IDB, Cache
* **Platform** → OS-specific wrappers
* **Sandbox** → Process isolation
* **AI Engine** → AI integrations only
* **Search Engine** → Query routing logic

**AI must NOT mix responsibilities.**

---

### ✔️ **Backward Compatibility**

AI-generated changes must never:

* Break existing public headers
* Rename core classes without explicit instruction
* Remove or alter subsystem boundaries
* Change CMake targets unless approved

---

### ✔️ **Minimalism**

AI code must be:

* Short
* Clean
* No unnecessary comments
* No repeating logic
* No unused variables
* No dead code

This keeps file size and readability high.

---

# 🚫 **3. Actions AI is NOT allowed to perform**

AI **must NOT**:

* Modify `source/zepraScript/` without explicit instruction
* Change folder structure
* Edit LICENSE
* Remove or rewrite WebCore foundational components
* Auto-format large parts of the codebase
* Mix UI code with engine code
* Generate non-standard naming conventions
* Introduce external dependencies not listed by the user

If unsure → AI must ask.

---

# 🔄 **4. Allowed AI Operations**

AI is allowed to:

### ✔️ Add new files inside correct module

e.g.,

```
source/webCore/layout/flow_layout.cpp
```

### ✔️ Fix bugs with minimal code changes

Only modify the exact lines needed.

### ✔️ Implement missing functions using existing architecture

### ✔️ Generate new modules when instructed

AI must follow directory structure perfectly.

### ✔️ Refactor code only in **small, isolated units**

---

# 🧠 **5. AI Understanding of File Responsibilities**

### 📌 `src/`

Browser application logic (UI, search bar, panels, etc.)

### 📌 `source/webCore/`

HTML → DOM → CSS → Layout → Rendering pipeline

### 📌 `source/zepraScript/`

Full JavaScript VM
*(AI should not touch unless Swanaya explicitly tells)*

### 📌 `source/networking/`

HTTP, DNS, TLS, caching

### 📌 `source/platform/`

Windowing, OS events, clipboard, notifications

### 📌 `source/aiEngine/`

AI logic, page analysis, smart actions, Gemini, Parth, Ripka integrations

### 📌 `source/devtools/`

Inspector UI and mechanics

---

# 🚀 **6. AI Change Workflow (AI MUST Follow)**

Here is the procedural flow an AI must follow before modifying code:

---

### **Step 1 — Understand Developer Intent**

AI must not assume requirements.
If user message is unclear → AI asks clarifying questions.

---

### **Step 2 — Identify Correct Module**

AI must locate the appropriate subsystem.

Example:
If user asks to modify request headers → go to `source/networking/http_request.cpp`.

If user asks to add flexbox stretching → go to `source/webCore/layout/flex_layout.cpp`.

---

### **Step 3 — Verify No Architectural Violations**

AI must cross-check changes with:

* modularity rules
* privacy rules
* platform separation
* dependencies

If a change violates these, AI must warn and ask for approval.

---

### **Step 4 — Generate Minimal Patch**

AI produces:

* only the changed code blocks
* not the entire file
* no comments like “add this”
* no explanations inside code
* no unrelated modifications

Example output:

```cpp
// Correct minimal patch
node->setDirty(true);
layout_tree->update(node);
```

---

### **Step 5 — Reconfirm Compatibility**

AI must ensure:

* no breakage of existing APIs
* code builds under C++20
* no undefined symbols
* correct CMake target remains valid

---

### **Step 6 — Provide Test Instructions**

AI must generate tests or usage notes if needed, e.g.:

```
Run: ./build/bin/ZepraBrowser --test-layout
```

---

# 🧪 **7. AI Testing Requirements**

AI-generated code must pass:

### ✔️ Unit tests

Located in `tests/unit/`

### ✔️ Integration tests

Located in `tests/integration/`

### ✔️ Web Platform Tests

Located in `tests/web_platform/`

### ✔️ Static Analysis

AI must ensure:

* no uninitialized values
* no memory leaks
* no undefined behavior

---

# 🔐 **8. AI Safety & Privacy Rules (Mandatory)**

### AI must not:

* Log user data
* Send browser content to external servers
* Introduce analytics/tracking
* Store personal information

Any external AI usage (Gemini, OpenAI) must pass through:

```
source/privacy/
```

---

# 🌐 **9. Multi-Model AI Interaction Rules**

AI engines must cooperate, not conflict:

* Parth AI handles real-time tasks
* RipkaAI handles heavy computation, coding, architecture
* AAdi handles image-gen and multimodal assets
* Browser AI Engine coordinates all of them
* External models are *optional* and must be sandboxed

---

# 🏗️ **10. Code Style (AI MUST Follow)**

### ✔️ Indentation: 4 spaces

### ✔️ No trailing spaces

### ✔️ Naming: `CamelCase` for classes, `snake_case` for functions

### ✔️ Headers: no unnecessary includes

### ✔️ No comments unless defining architecture (rare)

### ✔️ No logging unless user instructs to add diagnostics

---

# 🛑 **11. What To Do If AI Encounters Conflicts**

If AI cannot validate a safe patch:

AI must say:

```
This change may break the architecture.  
Please confirm or provide additional context.
```

---

# ⭐ **12. Golden Rule for AI in ZepraBrowser**

> **Never rewrite. Always extend.
> Never assume. Always confirm.
> Never break architecture. Always respect modularity.**

This rule alone keeps ZepraBrowser maintainable for decades.

---

# 📘 **13. Suggested AI Behavior Examples**

### When user says:

*“Add CSS variable support.”*

AI must respond:

* Confirm scope
* Only modify CSS parser, computed style, DOM style

### When user says:

*“Fix JS event bubbling.”*

AI must:

* Patch WebCore DOM event system
* NOT touch JavaScript engine

### When user says:

*“Add new search engine provider.”*

AI must:

* Update searchEngine module
* Update JSON configs
* Not touch UI unless instructed

---

# 🧬 **14. Final AI Self-Test Checklist**

Before ANY AI outputs code, it must ask itself:

✔ Am I modifying the correct subsystem?
✔ Am I respecting existing architecture?
✔ Is this the minimal patch?
✔ Did user explicitly request this?
✔ Will this break backward compatibility?
✔ Am I following Swanaya’s coding rules?

If all answers = **yes**, the AI may proceed.

---

# 🎉 **End of Guide**

Use this document as the **constitution for all AI modifications** in ZepraBrowser.

