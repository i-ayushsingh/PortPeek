# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

---

## Reporting a Vulnerability

PortPeek is designed with least-privilege principles:
- It queries process names using `PROCESS_QUERY_LIMITED_INFORMATION` without requiring Administrator privileges.
- It probes only loopback addresses (`127.0.0.1`) with non-blocking timeouts to prevent hanging or resource exhaustion.
- It stores configuration in user-scope registry (`HKEY_CURRENT_USER`).

If you discover a security vulnerability or privilege escalation bug in PortPeek:

1. **Do not open a public issue.**
2. Please open a [Private Vulnerability Advisory](https://github.com/i-ayushsingh/PortPeek/security/advisories/new) on GitHub.
3. Include details of the vulnerability, reproduction steps, and potential impact.

We will acknowledge your report within 48 hours and work on a prompt patch.
