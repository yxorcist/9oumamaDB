# 9oumamaDB

A small database engine I built from scratch in C.

The goal was mostly to understand how the different parts of a database fit together: storage, pages, indexes, transactions, concurrency, and eventually an HTTP server.

The rule I tried to follow while building it was:

> **Implement the important architectural ideas, but keep each implementation simple.**

## What it has

* persistent storage
* pages and page manager
* buffer pool
* LRU page replacement
* dirty pages
* page allocation/reuse
* hash table for key lookup
* BST for ordered/range queries
* temporary heap for TOPK
* INSERT / GET / UPDATE / DELETE
* RANGE / TOPK / COUNT
* BEGIN / COMMIT / ROLLBACK
* transaction snapshots
* request queue
* worker pool
* concurrent HTTP server
* JSON API
* tests
* AddressSanitizer / UBSan

## Architecture

```text
HTTP Client
     │
     ▼
HTTP Server
     │
     ▼
HTTP Connection
     │
     ▼
HTTP Request Parser
     │
     ▼
HTTP Dispatch
     │
     ▼
Request
     │
     ▼
Worker Pool
     │
     ▼
Request Queue
     │
     ▼
Database
  ┌──┴──────────────┐
  │                 │
HashTable           BST
  │                 │
  └───────┬─────────┘
          │
       Storage
          │
     Buffer Pool
          │
     Page Manager
          │
     database.db
```

TOPK is a bit different.

I originally had a permanent heap inside the DB, but that made updates annoying because the heap could become stale.

Now TOPK just builds a temporary heap when the query happens:

```text
Storage
   │
   ▼
Temporary Heap
   │
   ▼
TOP K entries
```

Much simpler.

## Entries

For now the database stores:

```c
typedef struct Entry {
    int key;
    int value;
    char nickname[NICKNAME_MAX];
} Entry;
```

Keys are unique.

The hash table is used for fast lookup by key and the BST is used when ordering matters, mainly for RANGE.

## Commands

```text
INSERT
GET
UPDATE
DELETE

RANGE
TOPK
COUNT

SAVE
LOAD
CLEAR

BEGIN
COMMIT
ROLLBACK
```

## HTTP API

The server runs on port `8080`.

### Insert

```http
POST /kv/1

{"value":100,"nickname":"alice"}
```

Response:

```json
{"status":"ok"}
```

Trying to insert the same key again gives `409 Conflict`.

### Get

```http
GET /kv/1
```

```json
{"key":1,"value":100,"nickname":"alice"}
```

Missing keys give `404 Not Found`.

### Update

```http
PUT /kv/1

{"value":500}
```

This changes the value and keeps the nickname.

### Delete

```http
DELETE /kv/1
```

### Count

```http
GET /count
```

```json
{"count":3}
```

### Range

```http
GET /range?a=10&b=50
```

Returns entries with keys between `10` and `50`.

### TOPK

```http
GET /topk?k=10
```

Returns the entries with the highest values.

### Transactions

```http
POST /transaction/begin
POST /transaction/commit
POST /transaction/rollback
```

Only one transaction can be active at a time.

## Storage

The storage path looks like this:

```text
Database
   │
   ▼
Storage
   │
   ▼
Buffer Pool
   │
   ▼
Page Manager
   │
   ▼
database.db
```

Page `0` stores metadata.

The pages after that store the entries.

Standalone INSERT, UPDATE, DELETE and CLEAR operations are persisted automatically.

When the DB starts, it loads `database.db` and rebuilds the hash table and BST.

LOAD is also done using temporary storage first:

```text
database.db
     │
     ▼
Temporary Storage
     │
     ▼
Temporary HashTable + BST
     │
     ▼
load worked?
   │       │
  yes      no
   │       │
   ▼       ▼
swap     keep old DB
```

So a bad LOAD doesn't destroy the DB that is already running.

## Transactions

BEGIN takes a snapshot of the entries and disables storage writes.

```text
BEGIN
  │
  ├── snapshot
  └── disable writes
          │
          ▼
      operations
       │      │
       ▼      ▼
    COMMIT  ROLLBACK
       │      │
       ▼      ▼
     save   restore
```

COMMIT enables writes and persists the new state.

ROLLBACK rebuilds the old state from the snapshot.

If rebuilding the rollback state fails, the current transaction state is kept instead of being half-destroyed.

If COMMIT fails, the transaction stays active so it can still be retried or rolled back.

## Concurrency

The server uses a request queue and worker pool.

```text
Client A ──┐
Client B ──┤
Client C ──┼──► HTTP Server
Client D ──┤
Client E ──┘
                │
                ▼
          Request Queue
           │    │    │
           ▼    ▼    ▼
          W1   W2   W3
            \   |   /
                ▼
                DB
```

The queue uses pthread mutexes and condition variables.

DB operations are protected by a mutex.

## HTTP

The HTTP implementation is intentionally pretty small.

One important thing I did handle is TCP fragmentation.

The server doesn't assume this:

```text
one read() == one HTTP request
```

It keeps reading until the headers and the expected request body have arrived.

Responses also escape nicknames before putting them inside JSON.

## Build

```bash
make
```

Run:

```bash
make run
```

Server:

```text
localhost:8080
```

## Tests

```bash
make test
```

Sanitizers:

```bash
make sanitize
```

For a full check:

```bash
make clean
make
make test
make sanitize
```

## Quick example

Start it:

```bash
make run
```

Insert something:

```bash
curl -i \
  -X POST \
  http://localhost:8080/kv/1 \
  -H 'Content-Type: application/json' \
  -d '{"value":100,"nickname":"alice"}'
```

Get it:

```bash
curl -i http://localhost:8080/kv/1
```

Update it:

```bash
curl -i \
  -X PUT \
  http://localhost:8080/kv/1 \
  -H 'Content-Type: application/json' \
  -d '{"value":500}'
```

TOPK:

```bash
curl -i 'http://localhost:8080/topk?k=10'
```

## v1 limitations

This is a learning project, not something I'd use for production.

Some things are intentionally simple.

### File format

Entries are currently written using their C struct representation.

That means `database.db` isn't guaranteed to be portable between different architectures, compilers, ABIs, endianness, etc.

### No WAL yet

There is no:

* WAL
* journaling
* copy-on-write
* real crash recovery

If the OS/disk fails in the middle of writing pages, the database file can end up partially written.

Fixing that properly would probably be one of the main things for a future version.

### Transactions are simple

Transactions use full in-memory snapshots.

There is no MVCC or fancy transaction manager.

Only one transaction can be active at a time.

### HTTP is simple

No:

* keep-alive
* chunked transfer encoding
* full HTTP implementation

Requests are also limited to 8 KiB.

### JSON is simple

I didn't bring in a full JSON library.

The parser only understands the JSON formats the API needs.

### Query results

RANGE and TOPK currently have a maximum result capacity of:

```text
100 entries
```

No pagination or streaming yet.

## Where this fits later

The DB is meant to stay generic.

If I build an actual application with it, the architecture would be:

```text
Frontend
   │
   │ HTTP / JSON
   ▼
Go Backend
   │
   │ HTTP / JSON
   ▼
9oumamaDB
   │
   ▼
database.db
```

So things like:

* users
* authentication
* sessions
* permissions
* game logic
* business logic

would belong in Go.

9oumamaDB just handles the database side.

## Why I built it

Mostly because I didn't want databases to just be:

```text
SQL query
   ↓
magic
   ↓
data
```

I wanted to actually build the layers underneath that abstraction at least once.

This is the first version where all those pieces are connected and working together.
