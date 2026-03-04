# Contributing to ZepraBrowser / ZebraScript

Thank you for your interest in contributing to the KetiveeAI ecosystem! ZepraBrowser and ZebraScript are built with a stringent focus on correctness, minimalism, and performance.

Before opening any pull requests or issues, please carefully review our **Engineering Operating Manual**.

## 1. Core Philosophy

**Primary Objective:** Deliver _correct, minimal, production-grade engineering output._

**Secondary Objectives:**

- Preserve system integrity
- Respect architectural intent
- Optimize for long-term maintainability
- Avoid unnecessary abstraction

**Non-Goals:**

- Teaching basics
- Over-commenting
- Decorative code
- Trend-driven rewrites

## 2. Code Standards

> [!CAUTION]
> Violating these standards is not a style issue — it is a correctness issue.

### 2.1 Minimalism First

```
No unnecessary comments.
No unused variables.
No placeholder logic.
No speculative abstractions.
```

If a line does not serve execution, correctness, or clarity — remove it.

### 2.2 Naming Conventions

Use **existing project naming exclusively**. Do NOT invent new naming styles. If a name is wrong, fix it to the established pattern.

- `Zepra*` - Browser-related modules
- CamelCase for methods/classes per existing conventions.

### 2.3 Language Discipline

- **Core Engine (ZebraScript)**: C++20 exclusively.
- **Browser Integrations**: C/C++.
- **Tooling / Build**: Python, Bash, CMake.

Never introduce web stacks (Electron, React, Node.js) for system-level functionality.

## 3. Workflow Expectations

**When modifying or generating code:**

1. Understand existing architecture.
2. Identify the minimal change.
3. Implement cleanly with thorough testing (update `tests/` accordingly).
4. Explain what changed and why clearly in your pull request.

**When debugging:**

1. Identify the root cause.
2. Propose the minimal fix.
3. No trial-and-error suggestions.

## 4. Architectural Respect

- Modify **only what is required**.
- Preserve existing execution flow unless explicitly told otherwise.
- Refactors must be incremental and justified.
- Avoid dynamic allocation in hot execution paths in the JIT/VM unless approved.
- Use GC handles properly; never assume raw pointer safety across allocations.

## 5. Security & Safety

All codebases are treated as **security-sensitive** by default.

- Assume hostile input from JavaScript execution.
- Validate bounds explicitly in all runtime buffers.
- Avoid unsafe defaults.

## 6. Submitting a Pull Request

1. Fork the repository and create your feature branch: `git checkout -b feature/my-new-feature`
2. Commit your changes. Ensure commit messages describe _why_ a change was made, not just _what_. Keep them professional.
3. Run the complete test suite locally: `make test` or `ctest`.
4. Run formatting checks.
5. Push to your branch and open a PR.

## 7. Final Rule

> **Simple & correct > Clever & complex**

This applies everywhere. No exceptions.
