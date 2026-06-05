# Security Policy

We take the security of AirTree seriously. Thank you for helping keep AirTree
and its users safe.

## Supported Versions

Security updates are provided for the most recent released version of AirTree.
We recommend always running the latest release.

| Version              | Supported |
| -------------------- | --------- |
| Latest release (1.x) | Yes       |
| Older releases       | No        |

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues,
pull requests, or discussions.**

Instead, report them privately by email to
[support@airmettle.com](mailto:support@airmettle.com). Where possible, please
include:

- A description of the vulnerability and its potential impact
- The AirTree version (`airtree --version`), and your operating system and
  architecture
- Steps to reproduce, ideally with a minimal proof of concept
- Any relevant logs, input-data characteristics, or stack traces

Please give us a reasonable opportunity to investigate and address the issue
before any public disclosure.

## What to Expect

- We will acknowledge your report as soon as we are able.
- We will investigate and keep you informed of our progress.
- Once the issue is resolved, we will coordinate with you on appropriate
  disclosure.

## Scope

This policy covers the AirTree source code and the official packages built from
it. AirTree depends on third-party libraries (for example Apache Arrow, Boost,
and OpenSSL); vulnerabilities in those projects are best reported to their
respective maintainers, though we are glad to hear about them so we can update
our dependencies.
