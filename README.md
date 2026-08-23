> **Implement the important architectural ideas, but keep each implementation simple.**

### Target architecture

```text
                    9oumamaDB
                        │
        ┌───────────────┼────────────────┐
        │               │                │
      Parser          Query            Server
        │             Engine             │
        │               │                │
        └────────────── DB ──────────────┘
                        │
             ┌──────────┼──────────┐
             │          │          │
          HashTable    BST        Heap
             │          │          │
             └──────────┼──────────┘
                        │
                     Storage
                        │
                  Buffer Pool
                        │
                  Page Manager
                        │
                   database.db
```

And eventually:

```text
Clients
   │
   │ TCP/HTTP
   ▼
9oumamaDB Server
   │
   ├── connection handling
   ├── authentication
   ├── request parsing
   └── concurrency
           │
           ▼
       Query Engine
           │
           ▼
       Transactions
           │
           ▼
      Storage Engine
```

### Major features I'd consider part of the project's eventual scope

**Already done:**

* persistent pages
* page manager
* buffer pool
* LRU
* dirty pages
* page allocation
* free-page reuse
* persistence
* hash table
* BST
* CRUD
* range queries
* parser/CLI
* tests

**Still worth implementing:**

1. **Heap / `TOPK`**
2. **Better query execution**
3. **Indexes**
4. **Transactions**
5. **Concurrency control**
6. **Locks**
7. **WAL / basic crash recovery**
8. **Database/server protocol**
9. **Multiple clients**
10. **Connection handling**
11. **Basic authentication**
12. **Stress/load testing**
13. **Benchmarking**
14. **Possibly an HTTP API**

### Concurrency specifically

We should eventually reach something like:

```text
Client A ──┐
Client B ──┤
Client C ──┼──► server ──► DB
Client D ──┤                │
Client E ──┘                ├── transaction A
                            ├── transaction B
                            └── transaction C
```

But we shouldn't jump directly into threads.

The progression should be:

```text
single-threaded DB
        ↓
transactions
        ↓
lock manager
        ↓
multiple concurrent operations
        ↓
server
        ↓
multiple client connections
        ↓
load testing
```
