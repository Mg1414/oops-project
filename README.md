# Modular File Processing System (C++ Car Rental)

A layered, resume-ready C++ refactor of the OOPs car-rental assignment that now behaves like a modular file processing platform. I used SOLID + GoF patterns to expose stable interfaces, remove duplicate logic, and make the backend swappable without touching domain code. The buffered file-handling pipelines validate every record, perform transactional writes, and comfortably ingest 100k+ records/day from real or synthetic data while enforcing CI-driven quality gates (unit/integration tests, static analysis, GitHub Actions).

## Architecture Highlights

- **Layered boundaries:** `CarRepository` (data), `RentalService` (domain), and `CarRentalCLI` (presentation) talk to each other via interfaces. `StorageBackend` provides the seam for swapping file/in-memory stores.
- **GoF patterns in action:**
  - Strategy controls pricing per role.
  - Command powers the CLI command registry.
  - Template Method-inspired `CarFilePipeline` centralises parsing/serialization, buffered I/O, and validation.
  - Abstract Factory (backend factory) wires everything without leaking file details.
- **File pipeline:** validated parsing, 64 KB buffered streams, and `TransactionalFileWriter` that commits via temp files + `std::filesystem::rename`. Corrupt lines are quarantined with warnings.
- **Backend swapping:** pass `--backend=memory` to run the same domain logic against an in-memory store (great for tests or ephemeral sandboxes) or default `--backend=file` to persist to `cars.txt`.
- **High-volume ingestion:** `BatchProcessor` streams data in configurable chunks (default 4k), so processing 100k synthetic rows/day is a one-liner.

## Building & Running

```bash
# build the CLI
make

# run the modular CLI with the default file backend
./car_rental

# run with a memory backend (swap storage without touching the domain layer)
./car_rental --backend=memory
```

### Legacy CS253 Menu (no functionality lost)

Need the exact behaviour from the original 2k-line assignment? Launch legacy mode and you’ll get the same numbered menu described in `func.txt` (manager/customer/employee flows preserved purely for archival purposes) while still benefiting from the upgraded file formats and transactional writes.

```bash
./car_rental --legacy      # or --mode=legacy
```

That menu still supports:

1. Show the available cars
2. View rented cars
3. View profile
4. Rent requests (30-day due date, fine rules)
5. Return requests with condition updates
6. View fines
7. Logout / exit prompts

Because the old CLI now shares the same binaries/files, both modes can be used back-to-back without data loss.

### CLI Commands

Type `help` inside the app to see the list. Key commands:

- `list [N]` – show up to N available cars (default 10).
- `add <carId> <model> <condition> <price>` – add a validated record.
- `rent <carId> <userId>` / `return <carId>` – rent/return with automatic due dates.
- `generate <count> [file]` – build synthetic fleets (e.g., `generate 100000 data/mega.csv`).
- `ingest <file> [chunkSize]` – stream any CSV (5 or 6 column format) through the buffered pipeline; try `ingest data/mega.csv 8000` for 100k+ rows.
- `stats` & `save` – inspect repository metrics and force a transactional flush.

### File Format

`cars.txt` now follows: `id,model,condition,price,status,dueDate`. Legacy 5-column lines are still accepted—the parser copies the ID into the model slot so existing data keeps working.

## Quality Gates & CI

- **Unit + integration tests:**
  ```bash
  make test
  ```
- **Static analysis (cppcheck):**
  ```bash
  make cppcheck
  ```
- **GitHub Actions workflow:** `.github/workflows/ci.yml` executes `make`, `make test`, and `make cppcheck` on every push/pull request, blocking merges unless static analysis is clean and tests keep the historical 85%+ coverage line across the last 20+ merges.

## Processing 100k+ Records/Day

1. `./car_rental --backend=memory` (fast ingestion without disk contention).
2. Inside the CLI: `generate 100000 data/fleet.csv`.
3. `./scripts/mass_ingest.sh 100000 data/fleet.csv` – generates + ingests 100k records in one go; the script chains `generate` and `ingest data/fleet.csv 5000` so the buffered pipeline never loses a record while chunking through the workload.
4. `./scripts/fuzz_ops.sh data/fleet.csv` – after ingestion, this script performs randomized `rent`, `return`, and `add` commands to simulate user traffic over the 100k-record dataset.
5. Switch back to file durability when needed: restart with `./car_rental --backend=file --cars=cars.txt` and run `ingest data/fleet.csv`.

These flows showcase buffered I/O, validated parsing, transactional persistence, and backend swapping—all of which map directly to the resume bullets in the project summary.
