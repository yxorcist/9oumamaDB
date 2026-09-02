# 9oumamaDB HTTP API

9oumamaDB exposes a small HTTP/JSON API.

The server runs on:

```text
http://localhost:8080
```

The API is intentionally simple. It is meant to be used by another backend, for example:

```text
Frontend
   │
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

9oumamaDB should be treated as a generic storage service.

Application logic, authentication, users, game rules, sessions, etc. should live in the application backend.

---

# Entry format

An entry contains:

```json
{
  "key": 1,
  "value": 100,
  "nickname": "alice"
}
```

Internally this corresponds to:

```c
typedef struct Entry {
    int key;
    int value;
    char nickname[NICKNAME_MAX];
} Entry;
```

## Constraints

### `key`

* integer
* must be `>= 0`
* unique

### `value`

* integer
* must be `>= 0`

### `nickname`

* string
* maximum 31 characters

---

# Insert

Creates a new entry.

```http
POST /kv/{key}
```

Example:

```http
POST /kv/1
Content-Type: application/json

{"value":100,"nickname":"alice"}
```

Example with curl:

```bash
curl -i \
  -X POST \
  http://localhost:8080/kv/1 \
  -H 'Content-Type: application/json' \
  -d '{"value":100,"nickname":"alice"}'
```

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

## Key already exists

```http
409 Conflict
```

```json
{"error":"key already exists"}
```

## Invalid request

```http
400 Bad Request
```

---

# Get

Reads one entry by key.

```http
GET /kv/{key}
```

Example:

```bash
curl -i http://localhost:8080/kv/1
```

## Success

```http
200 OK
```

```json
{
  "key":1,
  "value":100,
  "nickname":"alice"
}
```

## Key does not exist

```http
404 Not Found
```

```json
{"error":"not found"}
```

---

# Update

Updates the numeric value of an existing entry.

```http
PUT /kv/{key}
```

Body:

```json
{"value":500}
```

Example:

```bash
curl -i \
  -X PUT \
  http://localhost:8080/kv/1 \
  -H 'Content-Type: application/json' \
  -d '{"value":500}'
```

The nickname is not changed.

For example:

```json
{
  "key":1,
  "value":100,
  "nickname":"alice"
}
```

becomes:

```json
{
  "key":1,
  "value":500,
  "nickname":"alice"
}
```

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

## Key does not exist

```http
404 Not Found
```

```json
{"error":"not found"}
```

---

# Delete

Deletes an entry.

```http
DELETE /kv/{key}
```

Example:

```bash
curl -i \
  -X DELETE \
  http://localhost:8080/kv/1
```

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

## Key does not exist

```http
404 Not Found
```

```json
{"error":"not found"}
```

---

# Count

Returns the number of entries currently stored.

```http
GET /count
```

Example:

```bash
curl -i http://localhost:8080/count
```

Example response:

```json
{"count":3}
```

---

# Range

Returns entries whose keys are inside a range.

```http
GET /range?a={minimum}&b={maximum}
```

Example:

```bash
curl -i \
  'http://localhost:8080/range?a=10&b=50'
```

This returns entries where:

```text
10 <= key <= 50
```

Example response:

```json
[
  {"key":10,"value":120,"nickname":"a"},
  {"key":20,"value":300,"nickname":"b"},
  {"key":40,"value":50,"nickname":"c"}
]
```

The BST index is used for ordered range lookup.

## Constraints

```text
a >= 0
b >= 0
a <= b
```

The maximum number of returned entries is currently:

```text
100
```

---

# TOPK

Returns the entries with the highest values.

```http
GET /topk?k={count}
```

Example:

```bash
curl -i \
  'http://localhost:8080/topk?k=3'
```

Example database:

```text
key     value
1       50
2       500
3       200
4       1000
```

Request:

```http
GET /topk?k=2
```

Result:

```json
[
  {"key":4,"value":1000,"nickname":"four"},
  {"key":2,"value":500,"nickname":"two"}
]
```

TOPK builds a temporary heap from the current database entries.

The heap is destroyed after the query.

## Constraints

```text
1 <= k <= 100
```

---

# Transactions

9oumamaDB supports one active transaction at a time.

The transaction API consists of:

```text
BEGIN
COMMIT
ROLLBACK
```

---

# Begin transaction

```http
POST /transaction/begin
```

Example:

```bash
curl -i \
  -X POST \
  http://localhost:8080/transaction/begin
```

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

Starting another transaction while one is already active returns:

```http
409 Conflict
```

```json
{"error":"transaction already active"}
```

---

# Commit transaction

```http
POST /transaction/commit
```

Example:

```bash
curl -i \
  -X POST \
  http://localhost:8080/transaction/commit
```

COMMIT persists the changes made since BEGIN.

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

If no transaction exists:

```http
409 Conflict
```

```json
{"error":"no active transaction"}
```

If persistence fails, the transaction remains active.

The caller can then retry COMMIT or use ROLLBACK.

---

# Rollback transaction

```http
POST /transaction/rollback
```

Example:

```bash
curl -i \
  -X POST \
  http://localhost:8080/transaction/rollback
```

ROLLBACK restores the database state from before BEGIN.

## Success

```http
200 OK
```

```json
{"status":"ok"}
```

If there is no transaction:

```http
409 Conflict
```

```json
{"error":"no active transaction"}
```

---

# Transaction example

Start:

```bash
curl -X POST \
  http://localhost:8080/transaction/begin
```

Insert:

```bash
curl \
  -X POST \
  http://localhost:8080/kv/10 \
  -H 'Content-Type: application/json' \
  -d '{"value":100,"nickname":"temporary"}'
```

Update:

```bash
curl \
  -X PUT \
  http://localhost:8080/kv/20 \
  -H 'Content-Type: application/json' \
  -d '{"value":900}'
```

Then either commit:

```bash
curl \
  -X POST \
  http://localhost:8080/transaction/commit
```

or rollback:

```bash
curl \
  -X POST \
  http://localhost:8080/transaction/rollback
```

---

# Persistence

Standalone operations are automatically persisted:

```text
INSERT
UPDATE
DELETE
CLEAR
```

A normal write therefore looks roughly like:

```text
HTTP request
    │
    ▼
DB operation
    │
    ▼
change in-memory structures
    │
    ▼
persist storage
    │
    ▼
HTTP success
```

Inside a transaction:

```text
BEGIN
   │
   ▼
writes disabled
   │
   ▼
INSERT / UPDATE / DELETE
   │
   ▼
memory only
   │
   ├──────────────┐
   ▼              ▼
COMMIT          ROLLBACK
   │              │
   ▼              ▼
persist        restore snapshot
```

---

# HTTP behavior

The HTTP implementation is intentionally minimal.

Connections use:

```text
Connection: close
```

One request is handled per connection.

The server handles fragmented TCP requests and does not assume that one `recv()` contains the complete HTTP request.

---

# Request size

The maximum HTTP request size is currently:

```text
8192 bytes
```

Requests larger than this are not supported.

---

# JSON parsing

The JSON request parser is deliberately small.

It is not a general JSON parser.

For INSERT, use this structure:

```json
{"value":100,"nickname":"alice"}
```

For UPDATE:

```json
{"value":500}
```

It is safest for clients to generate exactly these structures.

For example, do not rely on arbitrary field ordering or advanced JSON syntax.

---

# Response JSON

Entry responses use:

```json
{
  "key":1,
  "value":100,
  "nickname":"alice"
}
```

Multiple entries are returned as arrays:

```json
[
  {"key":1,"value":100,"nickname":"alice"},
  {"key":2,"value":200,"nickname":"bob"}
]
```

Nickname strings are escaped when generating JSON responses.

---

# Status codes

The main status codes used by the API are:

| Status                      | Meaning                                   |
| --------------------------- | ----------------------------------------- |
| `200 OK`                    | Operation succeeded                       |
| `400 Bad Request`           | Invalid request, path, parameters or body |
| `404 Not Found`             | Requested key does not exist              |
| `409 Conflict`              | Operation conflicts with current DB state |
| `500 Internal Server Error` | Internal database/server operation failed |

---

# Error format

Errors are returned as JSON.

Example:

```json
{"error":"not found"}
```

Other examples:

```json
{"error":"key already exists"}
```

```json
{"error":"transaction already active"}
```

```json
{"error":"no active transaction"}
```

```json
{"error":"operation failed"}
```

---

# Client integration

A client does not need to understand the internal C implementation.

For example, a Go client could expose:

```go
type Entry struct {
    Key      int    `json:"key"`
    Value    int    `json:"value"`
    Nickname string `json:"nickname"`
}
```

with operations roughly like:

```go
Insert(key int, value int, nickname string) error

Get(key int) (Entry, error)

Update(key int, value int) error

Delete(key int) error

Count() (int, error)

Range(a int, b int) ([]Entry, error)

TopK(k int) ([]Entry, error)

Begin() error

Commit() error

Rollback() error
```

Internally these functions simply translate to the HTTP endpoints described above.

Example:

```text
Go:

db.Get(42)

        │
        ▼

GET http://localhost:8080/kv/42

        │
        ▼

9oumamaDB

        │
        ▼

{"key":42,"value":900,"nickname":"player"}
```

---

# Using 9oumamaDB in another project

A typical application should look like:

```text
Frontend
   │
   ▼
Application Backend
   │
   ├── users
   ├── authentication
   ├── sessions
   ├── validation
   ├── game logic
   └── business logic
   │
   ▼
9oumamaDB HTTP API
   │
   ▼
database.db
```

Do not put application-specific behavior into 9oumamaDB.

For example, the database should not know what these mean:

```text
player
game
match
card
user
password
winner
turn
```

Those concepts belong to the application backend.

From 9oumamaDB's point of view, it only stores generic entries.

---

# Current v1 limitations

9oumamaDB v1 does not provide:

* WAL
* crash recovery
* MVCC
* multiple simultaneous transactions
* full HTTP support
* HTTP keep-alive
* chunked transfer encoding
* full JSON parsing
* pagination
* streaming query results

A disk or OS failure during persistence may leave `database.db` partially written.

The current on-disk format also stores C structs directly, so database files are not guaranteed to be portable across architectures or incompatible builds.

These are known v1 limitations rather than guarantees provided by the API.
