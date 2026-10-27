# Autonomous Game Engine Architecture Rules

## Mandatory Golden Rule: Engine Design Thinking
- **Never Code Logic for Only One Actor**: Every system, feature, mechanic, physics resolution, movement calculation, camera mode, and AI behavior MUST be generic, component-driven, and usable across ANY actor in the engine.
- **No Hardcoded Actor Identifiers**: Never hardcode actor names, tags, IDs, model paths, or bone strings into core engine systems, camera modes, or collision solvers.
- **Component-Driven Data**: Expose properties through extensible components (e.g., RigidBodyComponent mass, ColliderComponent layers, CombatBlackBoard parameters), serialization, and editor controls so that any actor (player, minion, boss, NPC, environmental entity) can configure and use the behavior.
- **Physical Push & Mass Invariance**: Physical displacements between colliding actors must always be weighted by relative inverse masses rather than arbitrary 50/50 splits or hardcoded type exemptions. Heavy actors (e.g. high mass) naturally resist being pushed by lighter actors.
- **Maintain Documentation Integrity**: Preserve all existing comments and docstrings. Use bullet points exclusively (strictly zero numbered lists and zero numbered comments in code and responses).

## Hybrid Routing: Local GPU Model (local-qwen MCP)
- **Use Local Qwen for Discovery**: Call `local_qwen_file_analyzer` for first-pass scans of small and medium files (finding violations, listing functions, summarizing intent) to save cloud tokens.
- **Use Local Qwen for Boilerplate**: Call `local_qwen_coder` for routine drafts (structs, getters/setters, serialization blocks, inspector UI fields) before writing them by hand.
- **Always Verify Local Output**: Treat every local result as a lead, not a fact. Confirm line numbers, conditions, and semantics with grep or a direct file read before editing. The local model has inverted conditions (`!=` read as `==`) in past tests.
- **Respect the Size Limit & Use Slices**: The analyzer dynamically adapts to the local server context (16,384 tokens / ~45,000 characters) and explicitly reports any truncated range in a top `NOTE:`. For large files (e.g. `GameScene.cpp`), use `start_line` and `end_line` to analyze focused slices rather than sending full file reads to the cloud.
- **Cloud Model Owns Engine Edits**: Architecture decisions, cross-file refactors, math, shaders, and final file patches stay with the cloud model, followed by a build to verify.

## Collaboration Style: Design Thinking Talk
- **When Adding or Changing a System**: Name the technique first, explain why the current design falls short under the Golden Rule, explain how the technique works (math and data flow), show the proposed code, then pause so the user can decide whether to build it.
- **Teach the System**: Favor explanations that build the user's understanding of the engine architecture (strengths, violations, evolution path), not just the fix.
