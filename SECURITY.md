# Security

Tiny Engineer is a desk robot on your LAN. Firmware serves HTTP on port 80. Auth is optional.

## Report a vulnerability

Use [GitHub Security Advisories](https://github.com/jamro/tiny-engineer/security/advisories/new).

Do not file a public issue. Do not attach exploit PoCs or live tokens.

## In scope

- Auth bypass when an `access_token` is configured (JSON APIs should require `Authorization: Bearer <token>`)
- Token or Wi-Fi password leaked in `GET /settings`, HTML, or serial when `serial_log` is on
- Firmware RCE or unexpected code execution via the HTTP API

## Out of scope (by design)

- Calling `/anim` (or other control routes) on the LAN when no `access_token` is set — default is open on the local network
- First-boot setup AP `TinyEngineer-XXXX` with no password
- Physical access to the board, USB, or I2C
- 5 GHz Wi-Fi — the C3-Zero radio is 2.4 GHz only

## Auth behavior (intentional)

- Empty `access_token` (default): JSON APIs are unauthenticated
- Non-empty token: JSON APIs require `Authorization: Bearer <token>`; missing/wrong token → **401**
- `GET /auth` is always public and only reports whether auth is required
- HTML panel routes stay public (the UI prompts for the token)

See [docs/api.md](docs/api.md).
