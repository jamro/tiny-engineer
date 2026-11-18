# extension-serial Specification

## Purpose

Lets hosts drive extension REST surfaces over USB serial through one `ext` op that reuses the same mount table, access, and disabled-gate rules as HTTP `/extensions/<id>/…`.

## Requirements

### Requirement: Serial ext op mounts extensions
The serial control API MUST accept JSON requests with `op` equal to **`ext`**. The request MUST name a kebab-case extension `id`, a resource `suffix` (path segment after `/extensions/<id>/`), and an HTTP-equivalent `method` (`GET`, `POST`, `DELETE`, …). The device MUST dispatch using the same registered extension route table as HTTP.

#### Scenario: LCD text get over serial
- **WHEN** the host sends `ext` with `id` `lcd1602`, `suffix` `text`, and `method` `GET` while the extension is on
- **THEN** the response carries the same logical text JSON shape as `GET /extensions/lcd1602/text`

#### Scenario: Unknown extension id
- **WHEN** the host sends `ext` with an `id` that has no registered routes
- **THEN** the device answers **404** (or equivalent serial status) without performing extension hardware I/O

### Requirement: Ext refusals match HTTP
For a registered extension route, serial `ext` MUST apply the same access class as the HTTP route (`Configured`). When the extension’s settings toggle is false, `ext` MUST answer **503** with a clear disabled message and MUST NOT perform that extension’s hardware I/O. Unsupported method on a known suffix MUST answer **405**.

#### Scenario: LCD off refuses text post
- **WHEN** `lcd1602` is false and the host sends `ext` POST `text`
- **THEN** the response is **503** and no LCD I2C traffic runs for that request

#### Scenario: Wrong method
- **WHEN** the host sends `ext` with method `PUT` or `PATCH` on suffix `text` for `lcd1602`
- **THEN** the response is **405**

### Requirement: Ext body and success semantics match HTTP handlers
When method and body are valid for a registered route, serial `ext` MUST produce the same success/error codes and JSON payloads as the corresponding HTTP handler (including LCD validation **400**, missing hardware **503** messages, and last-state updates on success).

#### Scenario: Brightness set parity
- **WHEN** the host sends `ext` POST `brightness` with `{"brightness":50}` and the backlight is reachable
- **THEN** the response is **200** with brightness `50`, matching HTTP `POST /extensions/lcd1602/brightness`
