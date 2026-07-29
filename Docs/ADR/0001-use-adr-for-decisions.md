# ADR 0001: Use Architecture Decision Records (ADRs)

## Context
As the engine grows and multiple developers work on the codebase, we need a reliable way to communicate why specific technical choices were made. Without a history of decisions, developers may inadvertently reverse previous architectural choices or introduce inconsistencies.

## Decision
We will use Architecture Decision Records (ADRs) stored in `Docs/ADR/` to document any significant architectural, design, or structural choice. Every ADR will explain the context, the decision made, and the consequences.

## Consequences
- Better context retention across development sessions.
- Reduced friction and hallucination regarding project history.
- Minor overhead when making large decisions.
