# Engineering Nomenclature & Technical Lexicon Policy

This rule establishes strict terminology guidelines for all code, architecture documents, commit messages, and repository artifacts in the **H1OS** project.

## Disciplinary Grounding

We adhere strictly to professional **Embedded Systems Hardware and Software Engineering** standards.

Informal jargon, military slang, pop-culture metaphors, and ideological commentary are strictly prohibited. Always replace non-technical language with standard IEEE, ISO/IEC, and semiconductor industry nomenclature.

## Scope & Enforcement Boundary

1. **Mandatory Repository Artifact Scope:** This policy applies unconditionally to all content stored, persisted, or committed to the repository—including source code (`.c`, `.h`, `.cpp`, `.py`), inline comments, git commit messages, git tags, PR descriptions, documentation (`docs/`, `*.md`), and test ledgers.
2. **Interactive Chat Exemption:** Conversational brainstorming, casual dialogue, and informal banter within the interactive user chat are exempt from strict lexicon gating. However, the moment any text is written to disk, committed to version control, or generated as a repository artifact, it MUST strictly conform to the engineering lexicon.

---

## 1. Approved Engineering Vocabulary Mapping

| Avoid (Informal / Tactical Jargon) | Required Embedded Engineering Term | Engineering Context |
| :--- | :--- | :--- |
| **stand-down / stand down** | **sleep transition / power down / suspend** | Transitioning the SoC or peripherals into deep sleep or low-power state. |
| **sentinel / sentry / bouncer** | **monitor / supervisor / watchdog** | Coprocessor, task, or interrupt handler overseeing events or state. |
| **tripwire** | **threshold / limit trigger / brownout detector** | Voltage, thermal, or timer boundary triggering a state transition. |
| **sprint / sortie / mission** | **sample / sample burst / measurement pass** | Ultra-fast sensor acquisition pulse before thermal conduction. |
| **recon / reconnaissance** | **probing / polling / scanning / telemetry query** | Checking bus lines, I2C discovery, or sensor interrogation. |
| **payload** | **data buffer / telemetry frame / sample packet** | Sensor reading struct, ring buffer element, or network transmission. |
| **scaffold purge** | **refactoring / pruning / deprecation / removal** | Deleting obsolete legacy code and stubs. |
| **neutralize** | **clear / discharge / de-energize / reset** | Clearing display RAM, resetting DC bias, or de-asserting GPIO rails. |
| **kill / terminate (non-POSIX)** | **halt / disable / abort / power cut** | Graceful shutdown or gate de-assertion. |
| **fallback / fall back** | **default / recovery mode / safe state** | Fault handling logic. |
| **doctrine / dogma** | **standard / architecture / principle / specification** | Non-technical, cultish, or ideological framing. |
| **bones / the muscle / the vanity** | **contracts / subsystems / presentation layer** | Anatomical slang / informal construction metaphors. |

---

## 2. Technical Authoring Invariants

1. **Physical Accuracy:** Describe electrical and thermodynamic phenomena using exact physics terms (e.g., *quiescent current leakage*, *parasitic conduction*, *thermal gradient*, *die self-heating*, *open-drain pull-up*).
2. **Deterministic States:** Refer to state machine phases by their explicit functional names: *Flow A (Headless Sensor Sample Burst)*, *Flow B (Interactive UI Session)*, *Deep Sleep Standby*, *Storage Power-Off*.
3. **Hardware Registers & Signals:** Use explicit IC pin and register nomenclature per manufacturer datasheets (e.g., `ESP_SLEEP_WAKEUP_EXT1`, `PCF85063A Control_2`, `BAT_CTRL`, `SSD1681 Deep Sleep Mode 1`).

---

## 3. The Tone & Professionalism Clause ("Karen Clause")

To ensure an unassailably professional, objective, and safe engineering environment, all documentation, comments, commit messages, and agent outputs must strictly avoid language that could be misconstrued as aggressive, malicious, confrontational, or ideological:

1. **Prohibit Hostile & Aggressive Phrasing:**
   - Avoid violent, adversarial, or destructive idioms (e.g., *"nuke"*, *"blow away"*, *"destroy"*, *"wipe out"*, *"strangle"*, *"kill with prejudice"*). Use standard engineering equivalents (*"reset"*, *"erase"*, *"clear"*, *"deprecate"*, *"abort"*).
2. **No Manifesto-Style Prose:**
   - Do not write ideological declarations, polemics, grandiloquent treatises, or self-righteous philosophical rants.
   - Maintain a neutral, matter-of-fact, dispassionate tone focused purely on empirical measurement, circuit schematics, system architecture, and API contracts.
3. **Eliminate Provocative or Threatening Metaphors:**
   - Any turn of phrase that could remotely be perceived as hostile, ominous, punitive, or legally risky is forbidden.
   - Document technical deficiencies and errata objectively without derogatory characterizations, personal blame, or accusatory language.
4. **No Ordnance or Ordnance Disposal Terminology:**
   - Strictly prohibit all terms, metaphors, and analogies related to military ordnance, demolition, or ordnance disposal.
5. **No Pharmacological References:**
   - Strictly prohibit all pharmacological, medicinal, or toxicological references. Software and electrical faults MUST be described strictly using standard engineering nomenclature (*data corruption*, *memory leak*, *bus lockup*, *voltage brownout*).

---

## 4. Anti-Harassment & Non-Discrimination Policy

- Zero tolerance for language, comments, or naming that resembles bullying, harassment, discrimination, slurs, personal attacks, or hostile sarcasm.
- The repository must remain welcoming, professional, and accessible to the global open-source community.

---

## 5. Cultural, Religious, Mythical & Pop-Culture Prohibitions

Technical documentation must be timeless, factual, and free from external cultural baggage:

1. **No Superhero References:** No Batman, Superman, Avengers, Iron Man, Kryptonite, or comic book lore.
2. **No Movies, TV, or Song Quotes:** No pop-culture references, movie lines, lyrics, or meme catchphrases.
3. **No Religious or Biblical Terms:** Avoid scriptural and theological terms (*"heaven"*, *"hell"*, *"bible"*, *"gospel"*, *"miracle"*, *"salvation"*).
4. **No Fantasy or Mythical Jargon:** Strictly ban whimsical terms (*"magic"*, *"pixie dust"*, *"dragons"*, *"dragon farts"*, *"crystal balls"*, *"goblins"*). Note: *daemon* is permitted strictly in standard POSIX background service context.

---

## 6. Neutrality, Anti-Espionage & Anti-Obfuscation Policy

1. **No Espionage or Coded Speech ("Wind Talking"):**
   - No spy terminology, cryptic codenames, clandestine metaphors, or deliberately obfuscated double-meanings.
   - All technical prose must be plain, transparent, and direct.
2. **Strict Political & Ideological Neutrality:**
   - Completely prohibit political commentary, ideological labels (e.g., communism, socialism, capitalism, political parties), government grievances, or partisan discourse.
   - Completely prohibit racial references, categorizations, or generalizations.
   - The project is 100% focused on verifiable facts, electrical schematics, physical measurements, and mathematics.
