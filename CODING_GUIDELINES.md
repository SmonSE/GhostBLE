# GhostBLE Coding Guidelines

## Naming

### Classes
- Use PascalCase
- Example: DuplicateFilter, XPManager

### Functions
- Use camelCase
- Example: makeFingerprint(), getOrAssignDeviceId()

### Variables
- Use camelCase
- Example: deviceCount, isScanning

### Constants
- Use UPPER_CASE
- Example: MAX_DEVICES

---

## File Naming
- Use snake_case
- Example: duplicate_filter.cpp

---

## General Rules
- Avoid Arduino-specific types in core logic
- Prefer std:: types over Arduino types
- Keep hardware and logic separated

---

## Testing
- Core logic must be testable in native environment
- No NimBLE / Arduino dependencies in core/