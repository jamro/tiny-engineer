# Spec Delta

## ADDED Requirements

### Requirement: npm identity skips source content hashing
A client or daemon whose channel is `npm` MUST form its identity without hashing the on-disk contents of package source files. npm replace/keep decisions MUST continue to use protocol, channel, and SemVer only. The reported `sourceHash` for npm MAY be a stable empty or fixed sentinel shared by client and daemon from the same CLI generation.

#### Scenario: npm ensure does not depend on src bytes
- **WHEN** an npm client calls ensure against an npm daemon of the same SemVer
- **THEN** the CLI keeps that daemon even if package file bytes would differ between cache paths, and forming identity did not require reading those source files for a content hash

### Requirement: Dev source identity may be reused in-process
Within a single CLI or daemon process, a previously computed source identity for a given package root MAY be reused without re-reading package files. A new process MUST compute a fresh identity from disk. Dev replace-on-edit semantics for a later command after files change remain those of a new process seeing a different identity.

#### Scenario: Same process reuses source identity
- **WHEN** the same process loads identity for the same package root twice without clearing the identity cache
- **THEN** both loads report the same `sourceHash` without requiring a second full source walk

#### Scenario: New process sees edited checkout
- **WHEN** a dev daemon was started from a package root and a later new process from that root has different on-disk source bytes
- **THEN** that client’s identity differs and replace rules for an edited same-root checkout still apply
