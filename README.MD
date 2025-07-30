# Tentris: A Tensor-based Triple Store

Tentris is a tensor-based triplestore that natively supports worst-case optimal joins.

This is the research version of Tentris. A commercial version is available at https://github.com/tentris/tentris.

## Features

- SPARQL endpoint supporting `SELECT`, `SELECT DISTINCT` and `ASK` queries at `/sparql` and `/stream`. The `WHERE` block
  may contain basic graph patterns and OPTIONAL clauses.
  Queries must be well-designed [Kaminski et al. (2018)](https://doi.org/10.1007/s00224-017-9802-9)
- Supports `INSERT DATA`/`DELETE DATA` updates that are synchronized with a reader-writer lock at `/update`.
- Supports streaming results through the `/stream` endpoint for large results.
- Tentris is a persistent triplestore that uses [metall](https://github.com/LLNL/metall) to manage its disk-based index.
  Note that previous versions were in-memory.

## Download

We provide pre-built binaries in the [releases](https://github.com/dice-group/tentris-research-project/releases) that run on Linux machines with fairly recent (after 2015) x86_64 CPUs.

## Running

Initialize Tentris' index with an empty graph with:

```shell
./tentris_loader -f <(echo "")
```

Or initialize it by loading a turtle or ttl file:

```shell
tentris_loader --file my_nt_file.nt
```

Start the Tentris endpoint at [127.0.0.1:9080/sparql](http://127.0.0.1:9080/sparql):

```
tentris_server -p 9080
``` 

Run a query with :

```shell
curl -G \
--data-urlencode 'query=SELECT * WHERE { ?s ?p ?o . }' \
'127.0.0.1:9080/sparql'
```

Or update your data with:

```shell

curl -X POST \
-H 'Content-Type: application/sparql-update' \
--data 'INSERT DATA { <http://example.org/subject> <http://example.org/predicate> "object" . }' \
'127.0.0.1:9080/sparql'
```

The following endpoints are available:

- HTTP GET `/sparql` for normal SPARQL queries
- HTTP GET `/stream` for SPARQL queries with huge results
- HTTP POST `/update` for `INSERT DATA` and `DELETE DATA` updates

## Build It

Tentris is known to build on Ubuntu 22.04 and newer.
Building was tested with Clang 17 & 19. As C++ standard template library, only `libstdc++11` (v13) was tested. For
details
refer to the [Dockerfile](./Dockerfile) or GitHub actions.

# Research

- Alexander Bigerl, Liss Heidrich, Nikolaos Karalis and Axel-Cyrille Ngonga Ngomo (2025) **Efficient Updates for
  Worst-Case Optimal Join Triple Stores.** In: The Semantic Web – ISWC
  2025 | [website](https://github.com/dice-group/tentris-wcoj-with-updates/)

- Alexander Bigerl, Lixi Conrads, Charlotte Behning, Muhammad Saleem and Axel-Cyrille Ngonga Ngomo (2022) **Hashing the
  Hypertrie: Space- and Time-Efficient Indexing for SPARQL in Tensors.** In: The Semantic Web – ISWC
  2022 | [website](https://tentris.dice-research.org/iswc2022/)

- Alexander Bigerl, Felix Conrads, Charlotte Behning, Mohamed Ahmed Sherif, Muhammad Saleem and Axel-Cyrille Ngonga
  Ngomo (2020)
  **Tentris – A Tensor-Based Triple Store.**
  In: The Semantic Web – ISWC 2020 | [website](https://tentris.dice-research.org/iswc2020/)
