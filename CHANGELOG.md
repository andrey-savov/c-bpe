# Changelog

All notable changes to the c-bpe project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Restructured repository: C implementation promoted to primary, Rust moved to `rust/` subdirectory
- New repo identity: c-bpe (based on [rs-bpe](https://github.com/gweidart/rs-bpe) by gweidart)
- PyPI package name: `c-bpe`
- Unified CI workflow (`workflow.yml`) with dual C + Rust pipelines
- README rewritten to position C as primary implementation

## [0.1.0] - 2024-03-18
### Added
- Initial release (as rs_bpe)
- Python bindings for Rust BPE implementation
- Support for OpenAI tokenizers (cl100k_base and o200k_base)
- Comprehensive type hints with .pyi stub files
- Dynamic version management between Rust and Python
- CI/CD pipeline for testing and publishing 