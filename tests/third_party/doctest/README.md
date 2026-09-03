# doctest

This directory vendors the official doctest single-header distribution from
the latest Release used by this project:

- Version: `v2.5.3`
- Source: <https://github.com/doctest/doctest/releases/tag/v2.5.3>
- Header: `doctest/doctest.h`

The test targets use doctest in header-only mode. `test_main.cpp` defines
`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`; no doctest library is built or linked.
