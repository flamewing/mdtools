# Badges

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

[![CodeFactor Grade](https://img.shields.io/codefactor/grade/github/flamewing/mdtools?label=codefactor&logo=codefactor&logoColor=white)](https://www.codefactor.io/repository/github/flamewing/mdtools)
[![LGTM Alerts](https://img.shields.io/lgtm/alerts/github/flamewing/mdtools?logo=LGTM)](https://lgtm.com/projects/g/flamewing/mdtools/alerts/)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/flamewing/mdtools?logo=LGTM)](https://lgtm.com/projects/g/flamewing/mdtools/context:cpp)

[![CI Mac OS Catalina 10.15](https://img.shields.io/github/workflow/status/flamewing/mdtools/ci-macos?label=CI%20Mac%20OS%20X&logo=Apple&logoColor=white)](https://github.com/flamewing/mdtools/actions?query=workflow%3Aci-macos)
[![CI Ubuntu 20.04](https://img.shields.io/github/workflow/status/flamewing/mdtools/ci-linux?label=CI%20Ubuntu&logo=Ubuntu&logoColor=white)](https://github.com/flamewing/mdtools/actions?query=workflow%3Aci-linux)
[![CI Windows Server 2019](https://img.shields.io/github/workflow/status/flamewing/mdtools/ci-windows?label=CI%20Windows&logo=Windows&logoColor=white)](https://github.com/flamewing/mdtools/actions?query=workflow%3Aci-windows)

[![Coverity Scan Analysis](https://img.shields.io/github/workflow/status/flamewing/mdtools/coverity-scan?label=Coverity%20Scan%20Analysis&logo=data:image/svg+xml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMjU2IDI1MiIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNMjYuOTUgMTA5LjA4bC0zLjUyLTkuNDUgMzcuOTYgNzAuODloLjg1bDQ3LjMzLTExOC4xM2MuODMtMi41NiA4LjI2LTIxLjc0IDguNTEtMzAuMi42My0yMS44NC0xNC4xLTIzLjgxLTI5Ljc3LTE5LjM5QzM2Ljg3IDE5LjQ2LS4yNCA2Ny44My4wMSAxMjQuNzhjLjIgNTIuOTcgMzIuNjQgOTguMjQgNzguNjUgMTE3LjM4TDI2Ljk1IDEwOS4wOE0xNzQuMzMgNS40OGMtNi4zMiAxMi43LTEzLjEgMjYuMzctMjEuNjggNDguMDhMNzkuMjIgMjQyLjM5YzE1LjA5IDYuMiAzMS42MyA5LjYgNDguOTYgOS41MiA3MC41LS4yNyAxMjcuNDItNTcuNjcgMTI3LjEzLTEyOC4xOC0uMjItNTMuODMtMzMuNzYtOTkuNy04MC45OC0xMTguMjYiIGZpbGw9IiNmZmYiLz48L3N2Zz4=)](https://github.com/flamewing/mdtools/actions?query=workflow%3Acoverity-scan)
[![Coverity Scan](https://img.shields.io/coverity/scan/13716?label=Coverity%20Scan%20Results&logo=data:image/svg+xml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMjU2IDI1MiIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNMjYuOTUgMTA5LjA4bC0zLjUyLTkuNDUgMzcuOTYgNzAuODloLjg1bDQ3LjMzLTExOC4xM2MuODMtMi41NiA4LjI2LTIxLjc0IDguNTEtMzAuMi42My0yMS44NC0xNC4xLTIzLjgxLTI5Ljc3LTE5LjM5QzM2Ljg3IDE5LjQ2LS4yNCA2Ny44My4wMSAxMjQuNzhjLjIgNTIuOTcgMzIuNjQgOTguMjQgNzguNjUgMTE3LjM4TDI2Ljk1IDEwOS4wOE0xNzQuMzMgNS40OGMtNi4zMiAxMi43LTEzLjEgMjYuMzctMjEuNjggNDguMDhMNzkuMjIgMjQyLjM5YzE1LjA5IDYuMiAzMS42MyA5LjYgNDguOTYgOS41MiA3MC41LS4yNyAxMjcuNDItNTcuNjcgMTI3LjEzLTEyOC4xOC0uMjItNTMuODMtMzMuNzYtOTkuNy04MC45OC0xMTguMjYiIGZpbGw9IiNmZmYiLz48L3N2Zz4=)](https://scan.coverity.com/projects/flamewing-mdtools)

[![Windows snapshot build](https://img.shields.io/github/workflow/status/flamewing/mdtools/snapshots-windows?label=Windows%20Snapshot%20build&logo=windows&logoColor=white)](https://github.com/flamewing/mdtools/actions?query=workflow%3Asnapshots-windows)
[![Latest Windows snapshot](https://img.shields.io/github/release-date/flamewing/mdtools?label=Latest%20Windows%20snapshot&logo=windows&logoColor=white)](https://github.com/flamewing/mdtools/releases/latest)

## mdtools

Assorted tools for the Sega Mega Drive

## Create and install the package

You need a C++ development toolchain, including `cmake` at least 3.19 and [Boost](https://www.boost.org/) at least 1.54. You also need Git. With the dependencies installed, you run the following commands:

```bash
   cmake -S . -B build -G <generator>
   cmake --build build -j2
   cmake --install build
```

Here, `<generator>` is one appropriate for your platform/IDE. It can be any of the following:

- `MSYS Makefiles`
- `Ninja`
- `Unix Makefiles`
- `Visual Studio 16 2019`
- `Xcode`

Some IDEs support cmake by default, and you can just ask for the IDE to configure/build/install without needing to use the terminal.

## TODO

- [ ] Detail tools
- [ ] Use Boost::ProgramOptions
- [ ] Finish this readme
