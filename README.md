<p align="center">
  <picture>
    <img src="assets/banner.svg" alt="Nexora Airways" width="820">
  </picture>
</p>

<p align="center">
  <strong>A desktop flight-reservation system with a fluid QML interface and a strongly-typed C++20 reservation engine.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.5%2B-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6">
  <img src="https://img.shields.io/badge/QML-Quick-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt Quick / QML">
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++20">
</p>
<p align="center">
  <img src="https://img.shields.io/badge/SQLite-3-003B57?style=for-the-badge&logo=sqlite&logoColor=white" alt="SQLite 3">
  <img src="https://img.shields.io/badge/Argon2-password%20hashing-38bdf8?style=for-the-badge" alt="Argon2">
  <img src="https://img.shields.io/badge/Paystack-payments-011B33?style=for-the-badge" alt="Paystack">
</p>
<p align="center">
  <img src="https://img.shields.io/badge/CMake-3.21%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows">
  <img src="https://img.shields.io/badge/License-MIT-38bdf8?style=for-the-badge" alt="MIT license">
</p>

Nexora Airways is a full-featured airline booking application with a glass-styled QML interface backed by a strongly-typed C++20 reservation engine. It covers the complete booking lifecycle, from account creation and seat selection to secure online payments, across customer, admin and super-admin roles, with all data persisted in an embedded SQLite database.

## Features

| Feature | Description |
| --- | --- |
| Role-based access | Customer, Admin and Super-Admin (MAX) tiers, each with a tailored interface |
| Flight management | Browse, search, create and price flights across 21 Nigerian airports with live status (Scheduled, Boarding, Departed, Delayed, Cancelled) |
| Interactive seat maps | Visual seat selection with Economy, Business and First-Class cabins, including time-bounded cancellation rules |
| Integrated payments | Wallet top-ups via the [Paystack](https://paystack.com) API, plus withdrawals and peer-to-peer transfers |
| Secure authentication | Passwords hashed with **Argon2id** (PHC winner), salted per user and verified in constant time |
| Wallet and loyalty | Account balances, transaction history and loyalty-point accrual |
| Profiles | Editable personal details and next-of-kin records for customers and admins |

## Tech stack

| Layer | Technology |
| --- | --- |
| UI | Qt 6.5+ Quick / QML with a custom glassmorphic component set |
| Application | C++20 Qt Quick backend models exposed to QML |
| Core logic | Standalone C++20 reservation library (`reservation_core`) |
| Persistence | SQLite 3 (embedded, statically linked) |
| Security | Argon2id password hashing |
| Payments | Paystack REST API over Qt Network |
| Build | CMake 3.21+; MinGW on Windows, system Qt + SQLite3/Argon2 on Linux/macOS |

## Project structure

```text
backend/       Qt/QML bridge: Backend singleton, list and seat-grid models, Paystack client
core/          C++20 reservation engine: flights, seats, users, crypto, SQLite layer
qml/           QML views and reusable UI components
third_party/   Vendored Argon2 and SQLite3 sources plus the prebuilt archives CMake links
config/        Paystack secret and super-admin bootstrap files (created from *.example.txt)
data/          SQLite database (created at runtime)
icons/         Application icons and Windows resource
```

## Building

**Prerequisites:** Qt 6.5+ (with the Quick/QML modules), CMake 3.21+ and a C++20 compiler.

| Platform | Qt and dependencies |
| --- | --- |
| Windows | Qt 6.11 MinGW kit (default paths in the `mingw` preset) |
| Linux | `sudo apt install qt6-base-dev qt6-declarative-dev libsqlite3-dev libargon2-dev pkg-config ninja-build` |
| macOS | `brew install qt sqlite argon2 ninja` |

### Windows (MinGW)

```bash
cmake --preset mingw
cmake --build --preset mingw
```

The Windows build statically links the MinGW runtime and runs `windeployqt` automatically, producing a self-contained folder that runs on a clean Windows machine. Runtime data (database, payment config, icons) is copied next to the executable as part of the build.

> **Note:** On Windows, Argon2 and SQLite3 are compiled from source under `third_party/` using the same **MinGW-Builds 13.1.0 (MSVCRT)** toolchain as Qt. Mixing other prebuilt archives causes a startup load failure. Rebuild the archives with `third_party/build_thirdparty.sh` if needed.

### Linux / macOS

```bash
cmake --preset linux
cmake --build --preset linux
```

The executable is written to `build/linux/bin/NexoraAirways` (the `bin/` subdirectory avoids clashing with the same-named QML module directory). On these platforms the build links the **system** SQLite3 and Argon2 packages instead of the Windows archives, and the Windows-only pieces (`.rc` icon resource, `windeployqt`, static MinGW runtime flags) are skipped. The runtime icon deployment still works from `icons/nexora.ico`.

## Configuration

Configuration files are intentionally not committed. Copy the examples and fill in your own values:

```bash
cp config/paystack_secret.example.txt config/paystack_secret.txt
cp config/super_admin.example.txt config/super_admin.txt
```

| File | Purpose |
| --- | --- |
| `config/paystack_secret.txt` | Paystack secret key used for payment processing |
| `config/super_admin.txt` | Bootstrap password for the super-admin account |

Both files are read from the first line and are ignored by git, so real credentials stay out of the repository.

## License

Released under the [MIT License](LICENSE). Vendored third-party libraries retain their own licenses:

- [PHC winner Argon2](https://github.com/P-H-C/phc-winner-argon2) (CC0 / Apache-2.0)
- [SQLite](https://www.sqlite.org/) (public domain)
- [nlohmann/json](https://github.com/nlohmann/json) (MIT)

<p align="center"><sub>Built and maintained by <a href="https://github.com/gerald-mathew">Mathew Gerald Chukwudera</a></sub></p>
