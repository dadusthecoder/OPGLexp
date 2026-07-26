# Coding Style Guidelines

- **Namespaces**: Use `lgt::` for all engine code.
- **Naming Conventions**:
  - `PascalCase` for Classes, Structs, and Functions.
  - `camelCase` for local variables and parameters.
  - `m_PascalCase` for member variables.
  - `s_PascalCase` for static member variables.
- **Memory**: Prefer RAII. Avoid `new` and `delete` directly where smart pointers (`std::unique_ptr`, `std::shared_ptr`) can be used.
- **Modern C++**: Use C++17/23 features (`auto`, structured bindings, `constexpr`, `std::span`, `std::expected`).
