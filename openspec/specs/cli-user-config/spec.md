# cli-user-config Specification

## Purpose

Defines where the Tiny Engineer CLI stores user-level connection preferences, how those values are loaded relative to project overrides, and how setup persists them without mixing global and project storage.

## Requirements

### Requirement: User config directory and env file
The CLI MUST resolve a user config directory that is platform-appropriate: on Windows under `%APPDATA%\tiny-engineer` (or home if `APPDATA` is unset); on other platforms under `$XDG_CONFIG_HOME/tiny-engineer` or `~/.config/tiny-engineer`. If `TINY_ENGINEER_CONFIG_DIR` is set to a non-empty path, the CLI MUST use that directory instead. Connection preferences MUST be stored in a dotenv file named `env` inside that directory (not a leading-dot filename).

#### Scenario: Default Unix-like path
- **WHEN** the CLI runs on macOS or Linux without `TINY_ENGINEER_CONFIG_DIR` or `XDG_CONFIG_HOME`
- **THEN** the user env file path is under `~/.config/tiny-engineer/env`

#### Scenario: Config dir override
- **WHEN** `TINY_ENGINEER_CONFIG_DIR` points at a directory
- **THEN** the CLI reads and writes the user `env` file in that directory

### Requirement: Connection keys in user env
The user `env` file MUST be treated as the store for connection preferences `TINY_ENGINEER_TRANSPORT`, `TINY_ENGINEER_URL`, `TINY_ENGINEER_PORT`, and `TINY_ENGINEER_TOKEN` when those keys are present.

#### Scenario: Keys recognized from user env
- **WHEN** the user `env` file sets `TINY_ENGINEER_TRANSPORT=serial` and no higher-precedence source sets transport
- **THEN** later commands resolve transport as `serial`

### Requirement: Config load order for dotenv files
When filling unset connection keys from dotenv files, the CLI MUST load in this order (earlier wins): project-root `.env`, then `$CLAUDE_PROJECT_DIR/.env` when set, then the user `env` file, then `~/.gemini/config/.env`. Process environment already set MUST win over those files.

#### Scenario: Project overrides user
- **WHEN** project `.env` sets `TINY_ENGINEER_URL` to one value and the user `env` sets another
- **THEN** effective URL is the project value

#### Scenario: User fills when project absent
- **WHEN** project `.env` has no `TINY_ENGINEER_TRANSPORT` and the user `env` sets `serial`
- **THEN** effective transport is `serial`

### Requirement: Flags override env sources
Command-line flags that override URL, transport, or port MUST take precedence over process and file env sources for that invocation.

#### Scenario: Flag overrides files
- **WHEN** user `env` sets transport `serial` and the user passes `--transport http`
- **THEN** that invocation uses HTTP

### Requirement: No global-versus-project storage prompt for connection keys
The CLI MUST NOT ask the user whether a connection preference should be stored as global or project; connection keys always go to user `env`. IDE hooks are project-local unless `--global` is set or interactive setup answers `global`.

Interactive setup (no `--yes`, no `--global`) MUST ask which IDE before hook scope when no IDE id was given. The scope question MUST accept `global` or `project`, default `global`, and MUST come after the IDE choice. `--yes` MUST NOT ask. `--global` MUST NOT ask. Answering `global` MUST follow the same user-hook rules as the `--global` flag.

#### Scenario: Setup does not ask where to store connection keys
- **WHEN** the user runs setup and provides connection preferences
- **THEN** setup persists those keys to user `env` without prompting for a storage location for those keys

#### Scenario: Interactive setup asks IDE then hook scope
- **WHEN** the user runs setup without `--yes`, without `--global`, and without an IDE id
- **THEN** the IDE choice comes before the hook-scope prompt, and Enter on hook scope selects `global`

### Requirement: Setup labels global versus project destinations
During setup (interactive prompts, dry-run, and completion summary), the CLI MUST make clear which values are user/global connection preferences and which are project-level IDE hook installs. Connection-key messaging MUST name the user `env` destination (or equivalent user-config path). Hook messaging MUST name the project-relative config path(s), or the user hooks path when `--global` is set. The CLI MUST NOT imply connection keys are written into the project `.env`.

#### Scenario: Summary shows both scopes
- **WHEN** setup completes writing connection keys and hook configs
- **THEN** output distinguishes user/global connection prefs from project hook files

#### Scenario: Prompt labels connection as global
- **WHEN** interactive setup asks for transport, URL, port, or token
- **THEN** the prompt or adjacent message indicates the value is stored in user/global config

### Requirement: Setup writes connection keys only to user env
`tiny-engineer setup` MUST persist connection keys (`TINY_ENGINEER_TRANSPORT`, and when applicable `TINY_ENGINEER_URL`, `TINY_ENGINEER_PORT`, `TINY_ENGINEER_TOKEN`) only to the user `env` file. Setup MUST NOT create or modify the project `.env` for any reason. Setup MUST NOT prompt the user to choose global versus project storage for connection keys. IDE hook configuration MUST remain project-local unless the user passes `--global` or interactive setup answers `global`.

`--global` MUST write Cursor hooks only to the user file `~/.cursor/hooks.json` (home directory `.cursor/hooks.json` on every OS) and MUST NOT write that project's hook file. `--global` MUST write Claude Code hooks only to the user file `~/.claude/settings.json` (home directory `.claude/settings.json` on every OS) and MUST NOT write that project's `.claude/settings.json`. `--global` with `antigravity` or `--all` MUST exit with status 1, an error whose text contains `not supported` and the rejected integration ids, and MUST NOT write hook files. `uninstall --global` removes Tiny Engineer entries from the Cursor and Claude Code user files only. `uninstall --global` with `antigravity` or `--all` follows the same rejection rule and MUST NOT edit hook files.

#### Scenario: Global setup writes Cursor user hooks
- **WHEN** the user runs `setup cursor --global`
- **THEN** Cursor hook entries are merged into `~/.cursor/hooks.json` and the project `.cursor/hooks.json` is not created or modified

#### Scenario: Global setup writes Claude Code user hooks
- **WHEN** the user runs `setup claude-code --global`
- **THEN** Claude Code hook entries are merged into `~/.claude/settings.json` and the project `.claude/settings.json` is not created or modified

#### Scenario: Global setup rejects Antigravity and all
- **WHEN** the user runs `setup` with `--global` and `antigravity` or `--all`
- **THEN** the command exits with an error containing `not supported` and the rejected ids, and no hook file is written

#### Scenario: HTTP setup persists to user env
- **WHEN** the user completes setup with HTTP transport and a base URL
- **THEN** the user `env` file contains `TINY_ENGINEER_TRANSPORT=http` and `TINY_ENGINEER_URL` for that URL, and project `.env` is not given those keys by setup

#### Scenario: Serial setup persists to user env
- **WHEN** the user completes setup with serial transport and an optional port
- **THEN** the user `env` file contains `TINY_ENGINEER_TRANSPORT=serial` and, if a port was provided, `TINY_ENGINEER_PORT`, without requiring a URL

#### Scenario: Setup leaves project env untouched
- **WHEN** setup completes in a project that already has a `.env` file
- **THEN** that project `.env` file is left byte-identical (setup does not create, truncate, or rewrite it)

### Requirement: Setup warns when project overrides user config
When setup runs and project `.env` defines any connection key (`TINY_ENGINEER_TRANSPORT`, `URL`, `PORT`, `TOKEN`), setup MUST warn that those keys override user/global config and MUST name the keys. Setup MUST still write applicable keys only to user `env`.

#### Scenario: Setup warns on project URL override
- **WHEN** project `.env` sets `TINY_ENGINEER_URL` and the user runs setup
- **THEN** setup prints a warning that project `TINY_ENGINEER_URL` overrides user config

### Requirement: Override warnings never leak token values
When reporting a project `TINY_ENGINEER_TOKEN` override, setup and doctor MUST name the key only and MUST NOT print the secret value.

#### Scenario: Token override does not leak value
- **WHEN** project `.env` sets `TINY_ENGINEER_TOKEN` and setup runs
- **THEN** the warning mentions the token key without printing the secret

### Requirement: Doctor reports project connection overrides
Doctor MUST report when project `.env` supplies connection keys that override user config, naming those keys under the same token-redaction rules as setup.

#### Scenario: Doctor lists override keys
- **WHEN** project `.env` sets `TINY_ENGINEER_TRANSPORT` and the user runs doctor
- **THEN** doctor reports that project transport overrides user config

### Requirement: Confirm on different stored value
When interactive setup would write a connection key and the user `env` already has a different non-empty value for that key, setup MUST ask for confirmation before replacing it. If the stored value equals the proposed value, setup MUST leave it unchanged. Token prompts MUST NOT print the old or new token value.

#### Scenario: Interactive replace confirmed
- **WHEN** user `env` has `TINY_ENGINEER_URL=http://old` and interactive setup proposes `http://new` and the user confirms
- **THEN** user `env` is updated to `http://new`

#### Scenario: Interactive replace declined
- **WHEN** user `env` has a different URL and the user declines replace
- **THEN** the stored URL is left unchanged

### Requirement: Non-interactive setup keeps on conflict
When setup runs with `--yes` (or equivalent non-interactive mode) and the user `env` already has a different value for a connection key that setup would write, setup MUST keep the existing value and MUST NOT overwrite it. Setup MUST NOT require `--force-url` or `--force-transport` flags.

#### Scenario: Yes keeps conflicting transport
- **WHEN** user `env` has `TINY_ENGINEER_TRANSPORT=serial` and the user runs `setup --yes` with transport `http`
- **THEN** user `env` still has `TINY_ENGINEER_TRANSPORT=serial`

#### Scenario: Yes writes when absent
- **WHEN** user `env` has no `TINY_ENGINEER_TRANSPORT` and the user runs `setup --yes` with transport `serial`
- **THEN** user `env` contains `TINY_ENGINEER_TRANSPORT=serial`

### Requirement: Optional token in HTTP setup
When setup chooses HTTP transport and `--token` was not passed, it MUST offer an optional access token prompt. An empty skip MUST leave `TINY_ENGINEER_TOKEN` unset or unchanged in the user `env`. A provided token (flag or prompt) MUST be merged into the user `env` under the same confirm-on-diff / `--yes` keep rules as other connection keys. When setup chooses serial transport, it MUST NOT require a token.

#### Scenario: HTTP setup skips token
- **WHEN** the user chooses HTTP and leaves the token blank
- **THEN** setup completes without writing a new `TINY_ENGINEER_TOKEN` line unless one is being intentionally cleared by a documented clear action

#### Scenario: Serial setup does not require token
- **WHEN** the user completes serial transport setup
- **THEN** setup succeeds without collecting a Bearer token

### Requirement: Serial port cache uses user config directory
The serial port autodetection cache file MUST live in the same user config directory as the user `env` file (filename may remain `serial-port.json`).

#### Scenario: Cache under config dir
- **WHEN** autodetection caches a port
- **THEN** the cache file is stored under the resolved user config directory

### Requirement: Setup preserves --token when prompting
When HTTP interactive setup runs and `--token` was provided, setup MUST NOT replace that value with an empty prompt answer. Setup MUST prompt for a token only when `--token` is absent.

#### Scenario: Flag token survives Enter on prompt
- **WHEN** the user runs interactive setup with `--token secret` and presses Enter on the token prompt
- **THEN** setup still uses `secret` for persistence (subject to confirm-on-diff rules)

### Requirement: User env file has private permissions
When creating or updating the user config directory or `env` file on POSIX, the CLI MUST use private permissions (directory `0700`, file `0600`) and MUST tighten an existing user `env` file that is group- or world-readable when writing it.

#### Scenario: New user env is owner-only
- **WHEN** setup creates user `env` on POSIX
- **THEN** the file mode is owner-read/write only

### Requirement: Dotenv duplicate keys use first wins
When reading a key from a dotenv file for conflict detection or load, the CLI MUST use the first matching assignment (same rule as fill-if-unset load).

#### Scenario: First duplicate wins for conflict check
- **WHEN** user `env` has `TINY_ENGINEER_TRANSPORT=serial` then `TINY_ENGINEER_TRANSPORT=http`
- **THEN** conflict detection treats the stored value as `serial`

### Requirement: No cross-source token to project URL
The CLI MUST NOT send `Authorization: Bearer` with a token that came from user config or process env to a base URL that came only from project `.env`. Explicit CLI `--url` or process `TINY_ENGINEER_URL` remain allowed with a user or process token.

#### Scenario: Project URL does not receive user token
- **WHEN** project `.env` sets `TINY_ENGINEER_URL` and user `env` sets `TINY_ENGINEER_TOKEN` and no flag overrides the URL
- **THEN** HTTP requests do not send that Bearer token to the project URL

### Requirement: Warn when cross-source Bearer is withheld
When the CLI withholds a Bearer token under the cross-source rule, it MUST still attempt the HTTP request without the Authorization header and MUST emit a one-line stderr warning that names the reason (and relevant key names) without printing the token value. CLI help MUST mention this guard.

#### Scenario: Withheld token emits warning
- **WHEN** the CLI withholds a user or process token because the URL came only from project `.env`
- **THEN** stderr includes a one-line warning about the withhold and does not include the token value

#### Scenario: Help mentions the guard
- **WHEN** the user reads CLI help for config/auth
- **THEN** help notes that a user/process token is not sent to a project-only URL
