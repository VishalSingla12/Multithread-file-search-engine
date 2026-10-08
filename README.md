# Multithreaded File Search Engine

A production-quality **C++17** command-line application that recursively indexes a directory tree, builds an **inverted index**, and provides **TF-IDF ranked keyword search** — all powered by a custom thread pool and lock-free-friendly sharded data structures.

---

## Features

| Feature | Details |
|---|---|
| **Multithreaded indexing** | Configurable thread pool (`--threads N`); defaults to `hardware_concurrency()` |
| **Inverted index** | 64-shard design with per-shard `std::mutex` + `std::shared_mutex` document table |
| **TF-IDF ranking** | Normalized TF × smoothed IDF; deterministic scoring |
| **Thread-safe queue** | Generic `ThreadSafeQueue<T>` with graceful shutdown |
| **Index persistence** | Binary save/load with magic header, version, and length-prefixed strings |
| **Benchmarking** | Measures 1/2/4/8 threads; outputs `benchmarks/results.csv` |
| **Unit tests** | GoogleTest; covers tokenizer, queue, index, search, scanner, persistence |
| **Logging** | Lightweight singleton `Logger` (INFO / WARN / ERROR), file + console output |
| **CLI** | Argument-driven and interactive menu mode |

---

## Supported File Types

`.txt` `.md` `.cpp` `.h` `.hpp` `.c` `.py` `.java` `.json` `.csv` `.xml` `.yaml` `.yml` `.sh` `.bat` `.cmake` `.rs` `.go` `.js` `.ts` `.html` `.css` `.rb` `.swift` `.kt` `.scala` `.pl` `.lua` `.r` `.sql`

---

## Architecture

```
Directory Scanner (FileScanner)
        │
        ▼
Thread-Safe Task Queue (ThreadSafeQueue<T>)
        │
        ▼
Thread Pool (ThreadPool)
        │
        ▼
File Processing Workers (IndexManager::indexFile)
        │
        ▼
Tokenizer / Normalizer (Tokenizer)
        │
        ▼
Inverted Index (InvertedIndex — 64 shards)
        │
        ▼
Search Engine (SearchEngine — TF-IDF)
        │
        ▼
Ranked Search Results (SearchResult)
```

### Key Classes

| Class | Responsibility |
|---|---|
| `FileScanner` | Recursive `std::filesystem` traversal; produces `FileMetadata` |
| `FileMetadata` | Plain data struct (id, path, extension, size, mtime) |
| `ThreadSafeQueue<T>` | Generic MPMC queue with CV-based blocking and shutdown |
| `ThreadPool` | Fixed-size worker pool; `submit()` / `waitForAll()` / `stop()` |
| `Tokenizer` | Lower-case, strip punctuation, stop-word removal, min-length filter |
| `InvertedIndex` | Sharded `token → [Posting]` map + document table |
| `SearchEngine` | TF-IDF scoring, result ranking, query expansion |
| `SearchResult` | Score, matched terms, path, size |
| `IndexManager` | High-level coordinator: scan → pool → tokenize → index; persistence |
| `BenchmarkManager` | Multi-thread timing; writes `benchmarks/results.csv` |
| `Logger` | Singleton thread-safe logger (INFO/WARN/ERROR) |

---

## Step-by-Step Guide: How to Build & Run

### 🚀 One-Command Quickstarts

#### 🌟 Option 1: Modern Web UI (Streamlit Interface)
Run this single command to launch the full interactive browser interface:

**In PowerShell / VS Code Terminal:**
```powershell
python -m streamlit run app.py
```
**Or simply double-click / run:**
```cmd
run_gui.bat
```
This opens `http://localhost:8501` in your browser with:
- 📁 **Multi-Folder Concurrent Ingestion**: Ingest multiple directories simultaneously (e.g. `./data/sample`, `./include`, `./src`) into the C++ index with adjustable thread pool concurrency.
- 🔎 **Visual Keyword Search & Ranking**: TF-IDF ranked document cards displaying relevance scores, file extensions, and sizes.
- ✨ **Live Keyword Highlighting**: In-browser preview of matching files with search terms automatically highlighted in glowing yellow badges.
- 📈 **Real-Time Index Statistics**: Document count, unique tokens, and posting mappings.
- ⚡ **Interactive Benchmark Charts**: Concurrency speedup curves and files/sec throughput charts.

---

#### 💻 Option 2: Command-Line Interface (Console Menu)
Run the one-click CLI launcher:
```powershell
.\run.ps1
```
Or in CMD:
```cmd
run.bat
```

To compile and run this C++17 application, you need:
1. **CMake** (v3.16 or newer) &mdash; *Already installed on your system!*
2. **Ninja** or **Make** &mdash; *Ninja is already installed on your system!*
3. **C++17 Compiler**:
   - **Windows**: [Visual Studio 2022 Community](https://visualstudio.microsoft.com/vs/community/) (with the *"Desktop development with C++"* workload), **Visual Studio Build Tools**, or **MinGW-w64 (GCC 9+)** / **LLVM Clang**.
   - **Linux / WSL**: `sudo apt install g++ cmake ninja-build` (GCC 9+ or Clang 10+).

---

### Step 2: Build the Project

Open your terminal (PowerShell, Command Prompt, or bash) in the project directory:

```powershell
cd e:\multithreaded_file_search_engine
```

#### Option A: Using Visual Studio / MSVC (Recommended on Windows)
If Visual Studio 2019/2022 is installed:
```powershell
# 1. Configure the project
cmake -S . -B build

# 2. Build the executable and tests in Release mode
cmake --build build --config Release
```

#### Option B: Using MinGW-w64 GCC or Clang with Ninja
If modern GCC / Clang is in your PATH:
```powershell
# 1. Configure with Ninja generator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. Compile all targets
cmake --build build
```

---

### Step 3: Run Unit Tests

Execute the automated GoogleTest test suite:

- **Windows (Visual Studio / MSVC build)**:
  ```powershell
  .\build\bin\Release\run_tests.exe
  ```
- **Windows / Linux (Ninja or Make build)**:
  ```powershell
  .\build\bin\run_tests.exe
  # Or on Linux:
  ./build/bin/run_tests
  ```
- Or run via CTest:
  ```powershell
  ctest --test-dir build --output-on-failure
  ```

---

### Step 4: Run the Application

#### Interactive Mode (REPL Prompt & Menu)
```powershell
# Windows MSVC:
.\build\bin\Release\search_engine.exe

# Ninja / Linux:
.\build\bin\search_engine.exe
```

Once launched, you can type commands interactively:
```text
> index ./data/sample
> search machine learning
> stats
> help
> exit
```

#### Command-Line Mode (Direct Execution)
You can also run tasks directly via CLI flags:

1. **Index a directory and search immediately:**
   ```powershell
   .\build\bin\Release\search_engine.exe --index ./data/sample --search "machine learning"
   ```

2. **Index with a specific thread count (e.g., 4 threads):**
   ```powershell
   .\build\bin\Release\search_engine.exe --index ./data/sample --threads 4
   ```

3. **Display index statistics:**
   ```powershell
   .\build\bin\Release\search_engine.exe --index ./data/sample --stats
   ```

4. **Run concurrency benchmarks (1, 2, 4, 8 threads):**
   ```powershell
   .\build\bin\Release\search_engine.exe --benchmark ./data/sample
   ```
   *Results are automatically printed in a table and saved to `benchmarks/results.csv`.*

5. **Save the index to a file and reload it later:**
   ```powershell
   # Index and save to index.bin
   .\build\bin\Release\search_engine.exe --index ./data/sample --save-index index.bin

   # Load pre-built index and search without re-scanning disk
   .\build\bin\Release\search_engine.exe --load-index index.bin --search "thread pool"
   ```

---

```bash
# Index a directory with 8 threads
./search_engine --index ./documents --threads 8

# Search the index
./search_engine --index ./documents --search "machine learning"

# Show statistics
./search_engine --index ./documents --stats

# Run benchmark
./search_engine --benchmark ./documents

# Save / load the index
./search_engine --index ./documents --save-index index.bin
./search_engine --load-index index.bin --search "neural network"
```

### Full Options

```
  --index <dir>          Index all supported files in <dir>
  --search <query>       Search the current index
  --stats                Print index statistics
  --benchmark <dir>      Run multi-thread benchmark on <dir>
  --save-index <file>    Save the index to <file>
  --load-index <file>    Load the index from <file>
  --clear                Clear the in-memory index
  --threads <n>          Number of indexing threads (default: auto)
  --max-results <n>      Max search results to show (default: 10)
  --interactive          Launch interactive menu
  --help                 Show this help
```

---

## Scoring Algorithm (TF-IDF)

For query *Q* and document *d*:

$$\text{score}(d, Q) = \sum_{t \in Q} \underbrace{\frac{\text{tf}(t,d)}{\text{tf}(t,d)+1}}_{\text{normalised TF}} \times \underbrace{\ln\!\left(1 + \frac{N}{\text{df}(t)}\right)}_{\text{smoothed IDF}}$$

- **tf(t,d)** — raw count of term *t* in document *d*  
- **df(t)** — number of documents containing *t*  
- **N** — total document count  

This is a BM25-lite variant that does not require storing document lengths.

---

## Index Persistence Format

Binary format (little-endian, native byte order):

```
[Header]
  uint32  magic    = 0x4D465345  ('MFSE')
  uint32  version  = 1

[Per-document record × docCount]
  uint64  docCount
  uint64  id
  str     filename      (uint32 length + bytes)
  str     absolutePath
  str     extension
  uint64  fileSize
  int64   lastModified  (Unix epoch seconds)
  uint32  tokenCount
  str[]   tokens        (tokenCount × str)
```

---

## Benchmark Example Output

```
======================================================================
BENCHMARK RESULTS
======================================================================
Threads   Discovered  Indexed     Time(s)     Files/sec     Speedup
----------------------------------------------------------------------
1         12450       11823       8.41        1406          1.00x
2         12450       11823       4.73        2499          1.78x
4         12450       11823       2.61        4530          3.22x
8         12450       11823       1.89        6255          4.45x
======================================================================
```

Results are also written to `benchmarks/results.csv`.

---

## Repository Structure

```
multithreaded_file_search_engine/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── include/
│   ├── Logger.h
│   ├── FileMetadata.h
│   ├── ThreadSafeQueue.h
│   ├── ThreadPool.h
│   ├── Tokenizer.h
│   ├── FileScanner.h
│   ├── InvertedIndex.h
│   ├── SearchEngine.h
│   ├── SearchResult.h
│   ├── IndexManager.h
│   └── BenchmarkManager.h
├── src/
│   ├── main.cpp
│   ├── Logger.cpp
│   ├── FileMetadata.cpp
│   ├── ThreadPool.cpp
│   ├── Tokenizer.cpp
│   ├── FileScanner.cpp
│   ├── InvertedIndex.cpp
│   ├── SearchEngine.cpp
│   ├── IndexManager.cpp
│   └── BenchmarkManager.cpp
├── tests/
│   ├── test_tokenizer.cpp
│   ├── test_queue.cpp
│   ├── test_index.cpp
│   ├── test_search.cpp
│   ├── test_scanner.cpp
│   └── test_persistence.cpp
├── benchmarks/
│   └── results.csv
└── data/
    └── sample/
        ├── document1.txt
        ├── document2.md
        └── example.cpp
```

---

## C++17 Features Used

| Feature | Where |
|---|---|
| `std::filesystem` | `FileScanner`, `FileMetadata`, `IndexManager` |
| `std::thread` | `ThreadPool` |
| `std::mutex` / `std::lock_guard` | `InvertedIndex` shards, `Logger`, `ThreadSafeQueue` |
| `std::shared_mutex` / `std::shared_lock` | `InvertedIndex` document table |
| `std::unique_lock` | `ThreadSafeQueue::wait_and_pop` |
| `std::condition_variable` | `ThreadSafeQueue` |
| `std::atomic` | `ThreadPool` task counters, `FileScanner` doc-id generator |
| `std::unordered_map` | `InvertedIndex` posting lists |
| `std::priority_queue` | Available in `SearchEngine` ranking (currently std::sort) |
| `std::optional` | `ThreadSafeQueue::try_pop` / `wait_and_pop` |
| Structured bindings | `InvertedIndex::addDocument` (`for (auto& [token, freq])`) |
| `if constexpr` / fold expressions | Compiler-conditional platform code in `Logger` |

---

## License

MIT License — see [LICENSE](LICENSE).
