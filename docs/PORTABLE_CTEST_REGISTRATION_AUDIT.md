# Audit Portable C++20 Test Registrations

Run the fully offline source registry audit from the repository root:

\`\`\`powershell
python scripts/audit_portable_test_registration.py
\`\`\`

It inventories every \`Tests/Standalone/Portable*Tests.cpp\` test,
checks exactly one \`add_executable\` target per file in the portable
CMake project, verifies a matching runnable \`add_test\`, C++20,
\`/W4 /WX\` on MSVC and \`-Wall -Wextra -Wpedantic -Werror\` on GCC/Clang.

The auditor fails closed when a test is unregistered, missing, duplicated,
misnamed, linked or oversized. Its JSON receipt includes
\`compiled_or_executed=false\` and \`native_unreal_verified=false\`.
This is an **inventory checker**, not a compiler, code-coverage report,
authorization to merge Unreal gameplay, or a playable game certificate.

For real portable regression proof, use GitHub's exact-head C++ matrix
(Windows, Ubuntu, macOS). For actual gameplay, separately pass the trusted
UE5.8 native Editor/UHT, Win64 cook and installed game QA checklist.

Adversarial fixture tests live in
\`Tests/Editor/PortableRegistrationAuditTests.py\`; existing Python/editor
CI automatically discovers and runs them.
