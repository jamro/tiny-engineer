# extension-http Specification

## Purpose
Defines how optional hardware extensions expose REST under `/extensions/<id>/…`, including access checks and refusals when an extension is disabled, so later extensions can reuse one mounting pattern.

## Requirements

### Requirement: Extension routes use a shared path prefix
The firmware MUST mount every extension-owned REST route under `/extensions/<id>/<suffix>`, where `<id>` is a stable kebab-case extension identifier. The SPA shell route `GET /extensions` MUST remain the Extensions page and MUST NOT be treated as an extension API route.

#### Scenario: LCD text path shape
- **WHEN** a client calls the LCD 1602 text resource
- **THEN** the path is under `/extensions/lcd1602/` (not a top-level `/lcd1602` route)

#### Scenario: SPA Extensions page unchanged
- **WHEN** a browser requests `GET /extensions`
- **THEN** the device serves the web UI shell as today

### Requirement: Extension API access matches configured control routes
Extension API routes under `/extensions/<id>/…` MUST use the same access class as `POST /anim` (`Configured`: token when auth is on, and refused until connection mode is set).

#### Scenario: Unconfigured connection mode
- **WHEN** connection mode is not yet set and a client calls an extension API route
- **THEN** the device refuses with the same class of refusal used for `POST /anim`

### Requirement: Disabled extension refuses its API
When the settings toggle for extension `<id>` is false, every API route under `/extensions/<id>/…` MUST answer **503** with a clear disabled message and MUST NOT perform that extension’s hardware I/O for the request.

#### Scenario: Call while LCD extension off
- **WHEN** `lcd1602` is false and a client `POST`s `/extensions/lcd1602/text`
- **THEN** the response is **503** and the device does not talk to the LCD over I2C for that request

### Requirement: Unknown method on a known extension path is 405
For a path that belongs to a registered extension API route, an unsupported HTTP method MUST answer **405**. Paths that are not registered MUST answer **404**.

#### Scenario: Wrong method on text
- **WHEN** a client sends `PUT /extensions/lcd1602/text`
- **THEN** the response is **405**

### Requirement: Future extensions reuse the same mount table
New extension HTTP surfaces MUST be added by registering routes in the shared extension mount table (same prefix, access, and disabled gate), not by inventing a second top-level REST namespace for extensions.

#### Scenario: Second extension later
- **WHEN** a later change adds another extension with REST
- **THEN** its routes are mounted under `/extensions/<that-id>/…` using the same registry pattern
