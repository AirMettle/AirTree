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

AirTree is built from source via a single driver script that handles
dependency installation, configuration, compilation, testing, and packaging.

## Supported Platforms

| Platform        | Architecture     | Status              | Notes                  |
| --------------- | ---------------- | ------------------- | ---------------------- |
| macOS (Darwin)  | x86_64 / arm64   | ✅ Fully supported  | Requires Homebrew      |
| Ubuntu 22.04    | x86_64 / aarch64 | ✅ Fully supported  | Official CI target     |
| CentOS 7 / 8 / 9| x86_64           | ✅ Fully supported  | Uses `yum` + EPEL      |

Other Linux distributions may work if you manually satisfy the dependencies,
but only the above are exercised by the automated setup scripts.

## Prerequisites

### macOS

- [Homebrew](https://brew.sh) installed under `/opt/homebrew` or `/usr/local`.
- Xcode Command Line Tools (the build will prompt to install them if missing).

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

The build directory is isolated per compiler / version / build-type:

```
cmake-build-<compiler>-<version>-<buildtype>[-<sanitizer>]
```

For example, on Ubuntu 22.04 / x86_64 / gcc-11 / RelWithDebInfo with no
sanitizer: `cmake-build-gnu-11-RelWithDebInfo-nosan`.

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

## Next Steps

- [CLI Reference](cli.md) — using `airtree`, `airtree-export`, `airtree-merge`
- [C++ Library API](cpp-api.md) — programmatic use of the libraries
