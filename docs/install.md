# Installation & Build

[← Back to README](../README.md)

- [Supported Platforms](#supported-platforms)
- [Prerequisites](#prerequisites)
- [Recommended Build Command](#recommended-build-command)
- [Step-by-Step Options](#step-by-step-options)
- [Where Build Output Goes](#where-build-output-goes)
- [Installing the Built Package](#installing-the-built-package)
- [What Gets Installed](#what-gets-installed)
- [Putting AirTree on Your `PATH`](#putting-airtree-on-your-path)
- [Verifying the Install](#verifying-the-install)
- [Uninstalling](#uninstalling)
- [Troubleshooting](#troubleshooting)

AirTree is built from source via a single driver script that handles
dependency installation, configuration, compilation, testing, and packaging.

## Supported Platforms

| Platform        | Architecture     | Status              | Notes                              |
| --------------- | ---------------- | ------------------- | ---------------------------------- |
| macOS (Darwin)  | x86_64 / arm64   | Fully supported     | Requires Homebrew and `gcc@14`     |
| Ubuntu 22.04    | x86_64 / aarch64 | Fully supported     | Official CI target                 |
| CentOS Stream 9 | x86_64           | Fully supported     | Uses `dnf` + EPEL                  |

Other Linux distributions may work if you manually satisfy the dependencies,
but only the above are exercised by the automated setup scripts.

## Prerequisites

### macOS

- [Homebrew](https://brew.sh) installed under `/opt/homebrew` or `/usr/local`.
- Xcode Command Line Tools (the build will prompt to install them if missing).
- GCC 14 (`brew install gcc@14`). The macOS build hard-requires `gcc-14` /
  `g++-14` on `PATH` and will fail at configure time if they are missing.

### Linux (Ubuntu / CentOS)

- `sudo` access (or run as root).
- An internet connection (for package downloads).
- Basic tools: `git`, `curl`, `wget`, `unzip`.

The setup step installs the rest (GCC, CMake, Python 3.12, Ninja, etc.) using
the appropriate platform package manager.

## Recommended Build Command

From the repository root:

```bash
./tools/build/build.sh
```

This single command runs both stages:

1. **Setup** (`build.sh setup`) — detects your OS / architecture and installs
   all system dependencies.
2. **Build** (`build.sh airtree`) — creates a Python virtual environment,
   configures and builds with CMake, runs unit + functional tests, and
   produces installable packages.

## Step-by-Step Options

If you want finer control:

```bash
# Only install system dependencies
./tools/build/build.sh setup

# Only build, test, and package (after setup has run)
./tools/build/build.sh airtree

# Clean build artifacts
./tools/build/build.sh clean

# Clean only the cached third-party C++ dependencies
./tools/build/build.sh clean_deps
```

## Where Build Output Goes

The build directory is isolated per compiler / version / build-type / sanitizer:

```
cmake-build-<compiler>-<version>-<buildtype>-<sanitizer>
```

The sanitizer suffix is always present; the no-sanitizer build is spelled
`-nosan`. For example, on Ubuntu 22.04 / x86_64 / gcc-11 / RelWithDebInfo with
no sanitizer: `cmake-build-gnu-11-RelWithDebInfo-nosan`.

Third-party C++ dependencies are cached in:

```
<repo>/<build-directory>/.airmettle/airtree-deps/<host-arch-toolchain>/<dep-name>/
```

For example:
`<repo>/cmake-build-gnu-11-RelWithDebInfo-nosan/.airmettle/airtree-deps/ubuntu22-x86_64-gnu11-release/arrow_ep/`.

This dramatically speeds up subsequent builds.

Final installable packages produced by CPack land in the build directory:

- `.deb` (Ubuntu / Debian)
- `.rpm` (CentOS / RHEL)
- `.tar.gz` (always)

## Installing the Built Package

After a successful `./tools/build/build.sh`:

```bash
# Ubuntu / Debian
sudo dpkg -i cmake-build-*/airtree-*-Linux.deb

# CentOS / RHEL / Fedora
sudo rpm -i cmake-build-*/airtree-*-Linux.rpm
```

### Per-user install (no sudo)

If you cannot (or would rather not) install system-wide, the build also
produces a relocatable tarball alongside the `.deb` / `.rpm`. Extract it to
any writable prefix and prepend its `bin/` to `PATH`:

```bash
mkdir -p ~/.local/airtree
tar -xzf cmake-build-*/airtree-*-Linux.tar.gz -C ~/.local/airtree
export PATH="$HOME/.local/airtree/airtree-<version>-Linux/bin:$PATH"
```

The tarball lays out the same `bin/`, `lib/`, `include/airtree/` tree as the
system package.

## What Gets Installed

By default the package installs under a versioned prefix:

```
/opt/airmettle/airtree/<version>/
├── bin/
│   ├── airtree                # Generate + query CLI
│   ├── airtree-export         # Export tool
│   └── airtree-merge          # Merge tool
├── lib/
│   ├── libairtree-core.a
│   ├── libairtree-query.a
│   ├── libairtree-util.a
│   ├── libairtree-reader.a
│   ├── libairtree-merge.a
│   └── libairtree-export.a
└── include/
    └── airtree/               # Public headers
        ├── core/
        ├── query/
        ├── reader/
        ├── merge/
        └── export/
```

> **Note** — All AirTree libraries are **static archives** (`.a`). When linking
> your own code you must link the specific archives you need, plus their
> transitive dependencies. See [C++ Library API → Linking](cpp-api.md#linking).

## Putting AirTree on Your `PATH`

The default install prefix is *not* on `$PATH`. Add it to your shell rc:

```bash
export PATH="/opt/airmettle/airtree/<version>/bin:$PATH"
```

…or symlink the binaries into a directory that already is, e.g.:

```bash
sudo ln -s /opt/airmettle/airtree/<version>/bin/airtree /usr/local/bin/
sudo ln -s /opt/airmettle/airtree/<version>/bin/airtree-export /usr/local/bin/
sudo ln -s /opt/airmettle/airtree/<version>/bin/airtree-merge /usr/local/bin/
```

You can also override the prefix at configure time with
`-DCMAKE_INSTALL_PREFIX=/usr/local` if you prefer a system-wide install.

## Verifying the Install

Once `airtree` is on your `PATH`:

```bash
airtree --version
# AirMettle AirTree v1.3.0-SNAPSHOT

airtree generate csv -i examples/sales.csv -o /tmp/price.airtree -s 1DxP -c price
airtree query percentile -i /tmp/price.airtree -o /tmp/median.csv -p 50.0
cat /tmp/median.csv
# percentile,value
# 50,13.7509765625
```

If both commands run without errors, the install is good.

## Uninstalling

```bash
# Ubuntu / Debian
sudo dpkg -r airtree

# CentOS / RHEL / Fedora
sudo rpm -e airtree

# Per-user (tarball install)
rm -rf ~/.local/airtree
```

The system-package commands remove the binaries and headers but leave the
versioned prefix directory itself. Delete it manually if you want a fully
clean state: `sudo rm -rf /opt/airmettle/airtree/<version>`.

## Troubleshooting

**`./tools/build/build.sh` says "Platform not supported."**
Your OS / version isn't covered by the auto-setup scripts. See
[Supported Platforms](#supported-platforms). You can still build by installing
the dependencies manually (`gcc`, `cmake>=3.22`, `ninja`, `python3.12`,
`git`, `curl`, `unzip`) and running `./tools/build/build.sh airtree` to skip
the setup stage.

**Dependency download hangs or fails partway.**
Third-party C++ deps (Arrow, Boost, OpenSSL, …) are downloaded on first
build and cached under `<repo>/cmake-build-*/.airmettle/airtree-deps/`. If a
download is interrupted, the partial cache entry can confuse the next build.
Clear it with `./tools/build/build.sh clean_deps` and retry. Behind a corporate
proxy: make sure `https_proxy` / `http_proxy` are exported in the shell you
launch the build from.

**`airtree: command not found` after `dpkg -i`.**
The default install prefix is `/opt/airmettle/airtree/<version>/bin/`, which
is not on `$PATH` by default. See
[Putting AirTree on Your `PATH`](#putting-airtree-on-your-path).

**macOS build fails with "GCC-14 not found".**
The macOS build hard-requires GCC 14, not Apple Clang. Install it with
`brew install gcc@14` and ensure `gcc-14` / `g++-14` are on `PATH`.

**`Permission denied: /opt/airmettle/airtree`.**
You ran the system-package install (`dpkg -i` / `rpm -i`) without `sudo`, or
you tried `dpkg -i` in a container where `/opt` isn't writable. Either rerun
with `sudo`, or use the per-user tarball install described
[above](#per-user-install-no-sudo).

## Next Steps

- [CLI Reference](cli.md) — using `airtree`, `airtree-export`, `airtree-merge`
- [C++ Library API](cpp-api.md) — programmatic use of the libraries
