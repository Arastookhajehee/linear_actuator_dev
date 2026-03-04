# AGENTS.md
Guidance for coding agents working in this repository.

## 1) Repository Snapshot
- Core files: `hello_bullet.py`, `simple_robot.urdf`, `meshes/*.stl`.
- Stack: Python script using PyBullet + URDF robot model.
- Current tooling state (detected):
  - No build config (`Makefile`, `pyproject.toml`, `tox.ini`, etc.)
  - No lint/format config
  - No test config
- Implication: use explicit commands from this document.

## 2) Environment
- Python 3.10+ recommended.
- Windows paths are first-class in this repo.
- Runtime dependency: `pybullet`.
- Optional dev tools: `pytest`, `ruff`, `black`.

## 3) Setup Commands
Run from repo root (`simple_robot/`).
```bash
python -m venv .venv
```
```bash
.venv\Scripts\activate
```
```bash
python -m pip install --upgrade pip
python -m pip install pybullet pytest ruff black
```
PowerShell activation alternative:
```bash
.\.venv\Scripts\Activate.ps1
```

## 4) Build / Run / Lint / Test Commands
There is no compile/build step in the current codebase.
Run simulation:
```bash
python hello_bullet.py
```
Lint (preferred):
```bash
ruff check .
```
Format (preferred):
```bash
black .
```
Run all tests:
```bash
pytest -q
```
Run a single test (primary pattern):
```bash
pytest tests/test_example.py::test_case_name -q
```
Other useful test selectors:
```bash
pytest tests/test_example.py -q
pytest -k "keyword_expr" -q
```
If stdlib `unittest` is used:
```bash
python -m unittest tests.test_example.TestClass.test_method
```

## 5) Validation for URDF / Mesh Changes
After editing `simple_robot.urdf` or files in `meshes/`:
1. Run `python hello_bullet.py`.
2. Confirm the model loads without missing mesh errors.
3. Verify expected joint behavior and limits.
4. Keep relative mesh paths stable unless intentionally migrated.

## 6) Code Style Guidelines
Apply to all new or modified code.

### Imports
- Order imports: standard library, third-party, local.
- Prefer explicit imports; do not use wildcard imports.
- Remove unused imports.
- Use one import per line unless grouped from-import improves readability.

### Formatting
- Follow PEP 8.
- Use `black` formatting defaults.
- Line length target: 88.
- Use 4 spaces for indentation.
- Ensure newline at end of file.

### Types
- Add type hints to new functions/methods.
- Include return annotations for non-trivial logic.
- Prefer concrete, accurate types at API boundaries.
- Do not add speculative or incorrect annotations.

### Naming
- Modules/files: `snake_case.py`.
- Variables/functions: `snake_case`.
- Classes: `PascalCase`.
- Constants: `UPPER_SNAKE_CASE`.
- Internal helpers: `_leading_underscore`.

URDF naming conventions:
- Preserve existing link/joint names when possible.
- Name new links/joints consistently and descriptively.
- Avoid renaming identifiers that external scripts may reference.

### Error Handling
- Fail fast with clear exception messages.
- Avoid bare `except:` blocks.
- Catch specific exceptions when recovery is meaningful.
- Do not silently swallow runtime failures.
- Validate critical file paths before starting simulation loops.

### Structure and Runtime Behavior
- Keep top-level script logic thin; extract reusable functions.
- Replace magic numbers with named constants.
- Prefer deterministic test behavior (`p.DIRECT` where practical).
- Make simulation mode explicit (`p.GUI` vs `p.DIRECT`).

### Comments and Documentation
- Add comments only for non-obvious intent.
- Keep comments synchronized with current behavior.
- Use concise docstrings for reusable functions.

## 7) Testing Guidelines
- Place tests in `tests/`.
- Name files `test_*.py`.
- Name test cases by behavior, not implementation detail.
- Prefer fast, headless PyBullet tests.
- Ensure physics clients are cleaned up per test.

## 8) Agent Operating Rules
- Make minimal, targeted edits.
- Do not rename/move mesh assets unless requested.
- If tooling changes, update commands in this file.
- If CI is introduced, keep local and CI commands aligned.

## 9) Cursor / Copilot Rules
Checked for external agent-instruction files:
- `.cursor/rules/`
- `.cursorrules`
- `.github/copilot-instructions.md`

Result at time of writing:
- No Cursor rules found.
- No Copilot instruction file found.

If these files are added later, merge their requirements into this document.
