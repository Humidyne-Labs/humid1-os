# Markdown and GFM Formatting Rules

All public documentation (`.md`) across this repository must strictly adhere to the GitHub Flavored Markdown (GFM) specification.

> [!NOTE]
> **Scope & Exemption Boundary:** Strict GFM rules apply exclusively to public-facing documentation (`README.md`, `ERRATA.md`, `CHANGELOG.md`, `docs/architecture/`, `docs/drivers/`, `docs/pinout.md`, etc.). Internal agent scratch notes, telemetry dumps, and working artifacts (`docs/agent_notes/`, `docs/logs/`, brain artifacts) are exempt from strict GFM syntax to avoid rendering distortion.

Refer to the skill [`.agents/skills/gfm/SKILL.md`](../skills/gfm/SKILL.md) for full reference.

## Mandatory Rules

1. **MathJax Block Math (`$$`):**
   - MUST be isolated on separate lines starting strictly at **column 0** (zero leading spaces, even when following a bulleted or numbered list item). GitHub's `cmark-gfm` parser breaks indented `  $$` blocks inside lists into plain text!
   - MUST be preceded and followed by an empty blank line.
   - NEVER format block math on a single line (`$$formula$$` is forbidden).

2. **MathJax Inline & Block Subscripts (`$` / `$$`):**
   - No whitespace between the opening/closing `$` and the expression (`$x$` is valid, `$ x $` breaks MathJax).
   - **NEVER use `\_` inside `\text{...}` or math expressions:** GitHub's Markdown preprocessor strips the backslash from `\_` before passing the expression to MathJax, causing a fatal `'_' allowed only in math mode` error. Use single-word subscripts (`T_{\text{floor}}`), hyphens (`T_{\text{amb-floor}}`), or parentheses (`V_{\text{bat(mV)}}`).
   - Use `\text{...}` for units: `$\mu\text{A}$`, `\text{ms}`, `^\circ\text{C}`.
   - Escape literal dollar signs with `\$` or wrap in backticks.

3. **Mermaid Diagrams:**
   - Use canonical `graph TD;` or `graph LR;` syntax with explicit semicolons (`;`) at the end of statements (avoid `stateDiagram-v2` transition labels with parentheses or arrows, which fail on GitHub).
   - Always declare subgraphs with alphanumeric identifiers: `subgraph ID ["Label"]`.
   - Wrap all node text in double quotes `["..."]` and edge labels in `-- "Label" -->`.
   - **NEVER use raw HTML tags (`<br/>`), raw ampersands (`&`), nested square brackets (`[...]`), bullet characters (`•`), or non-ASCII Unicode symbols (`Ω`, `µ`, `–`) inside Mermaid labels.** Use plain ASCII (` - `, `and`, `uA`, `Ohm`) instead.
   - Never use reserved keywords (`end`, `subgraph`, `graph`, `style`) as node or subgraph IDs.
   - Avoid duplicate IDs between nodes and subgraphs.

4. **Alerts & Callouts:**
   - Use strictly `> [!NOTE]`, `> [!TIP]`, `> [!IMPORTANT]`, `> [!WARNING]`, or `> [!CAUTION]`.
   - No leading spaces before the `>`.

5. **HTML and Markdown Interleaving:**
   - Always insert an empty newline after `</summary>` or closing HTML tags before writing Markdown.
