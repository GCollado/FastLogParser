# FastLogParser: C++ Enterprise Log Extractor

A high-performance C++ command-line utility that scans massive system log files, extracts flagged events, and persists them into a SQLite relational database for automated auditing and QA analysis.

## The Problem
Enterprise clinical platforms—such as Epic Beaker—generate gigabytes of daily log files during batch synchronizations. When records drop to exception queues (e.g., a `MISSING_MD` signature), manually parsing unstructured text to isolate these failures is inefficient.

This tool automates the extraction pipeline, transforming unstructured system logs into a queryable relational database in milliseconds.

## Key Features
* **Optimized I/O (Transactions):** Wraps SQLite inserts in bulk transactions (`BEGIN TRANSACTION;`), committing hundreds of thousands of parsed lines to disk almost instantly.
* **Memory-Efficient:** Processes files line-by-line, maintaining a flat memory footprint regardless of whether the target file is 10 MB or 10 GB.
* **Defensive Parsing:** Validates string boundaries to prevent runtime crashes from malformed data and safely strips cross-platform line endings (Windows CRLF).
* **Verbose Mode (`-v`):** Toggles real-time console output for manual troubleshooting, while defaulting to a silent, fast summary mode for automated background tasks.
* **Zero-Configuration Database:** Integrates the SQLite C-API directly. The resulting `.db` files can be queried immediately using standard SQL or Python (Pandas) without a dedicated database server.

## Technology Stack
* **Language:** C++17
* **Database:** SQLite3 (C-API)
* **Build:** Standard `g++` or CMake

## Installation

Clone the repository and compile. The SQLite source (`sqlite3.c`) is included directly for seamless compilation without external package managers.

```bash
git clone [https://github.com/yourusername/FastLogParser.git](https://github.com/yourusername/FastLogParser.git)
cd FastLogParser
g++ main.cpp sqlite3.c -o log_parser -pthread -ldl


UsageSyntax:

Bash
./log_parser -i <input_log_file> -k <search_keyword> [-v]

Automated Run (Fast & Silent):



Bash
./log_parser -i system_events.log -k MISSING_MD

Output:
Plaintext
Scanning logs and writing to SQLite database...
Database sync complete. Inserted 2 records into parsed_logs.db.


Developer Run (Verbose):
Bash
./log_parser -i system_events.log -k MISSING_MD -v

Output:
Plaintext
Scanning logs and writing to SQLite database...
 -> Found: 2026-08-07 08:15:33 [ERROR] Record ID AP-88304: Validation failed - MISSING_MD signature on surgical pathology report. Routing to exception queue.
 -> Found: 2026-08-07 08:17:42 [ERROR] Record ID AP-88307: Route failure - MISSING_MD attending provider NPI for send-out test.
Database sync complete. Inserted 2 records into parsed_logs.db.


Database Schema

The generated parsed_logs.db maps unstructured strings to the following queryable structure:


ID(PK)  TIMESTAMP               ERRORLEVEL      MESSAGE
1       2026-08-07 08:15:33     ERROR           Record ID AP-88304: Validation failed...
2       2026-08-07 08:17:42     ERROR           Record ID AP-88307: Route failure - MISSING_MD...
