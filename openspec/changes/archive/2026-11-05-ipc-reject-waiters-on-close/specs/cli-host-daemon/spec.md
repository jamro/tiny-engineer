# Spec Delta

## ADDED Requirements

### Requirement: IPC request fails when the connection closes
While a CLI client is waiting for a daemon IPC response, if that connection closes or errors before a response arrives, the waiting request MUST fail promptly with an error that states the connection closed. It MUST NOT remain pending solely until the request timeout. A timeout MUST still apply when the connection stays open but no response arrives.

#### Scenario: Socket closes during an in-flight request
- **WHEN** a client has an outstanding IPC request and the daemon socket closes before a response
- **THEN** that request fails with a connection-closed error without waiting for the full request timeout

#### Scenario: Alive peer still times out
- **WHEN** a client has an outstanding IPC request, the socket stays open, and no response arrives before the request timeout
- **THEN** the request fails with a timeout error as today
