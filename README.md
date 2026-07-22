# TinyGit

> A byte-for-byte from-scratch implementation of Git written in C.

## Overview

TinyGit is a smaller version of Git that recreates the most basic git commands developers use to learn why Git is built the way it is. This is for anyone who wants to see what Git actually does behind the scenes when you first start your repo, and up to when you actually commit your changes. What makes TinyGit special is that YOU (yes, you) can verify TinyGit's implementation with real Git.

## Motivation

There are a few reasons why I wanted to build TinyGit, so let's dive in!

1. Git is beautiful, and what makes it beautiful is its simplicity. I wanted to learn this simplicity, as I believe it's important to learn the WHY in its systems. (Did you know that 94% of developers use Git?)
2. I wanted to get really good at C. This might sound basic, but let me explain. My interests lie in aerospace and embedded engineering. Therefore, for me to get where I need to be, I need to be skilled in C.
3. As mentioned earlier, I use Git all the time. I wanted to get better with Git, so I decided to get deep in the plumbing to also learn Git at a deep level.

Overall, I wanted to get better at implementing systems, writing C code, and using Git.

## Features

Below are the commands you can use in TinyGit:

- `tinygit init` — initialize a repository
- `tinygit add <file>` — stage file contents
- `tinygit commit -m <msg>` — create a commit
- `tinygit log` — walk the commit history

## Architecture

TinyGit models a repo in two layers:

- an immutable object store
- mutable pointers to that object

The immutable object store is CONTENT-ADDRESSABLE, meaning that the content, once hashed, is the key. This means we NEVER modify an object in place. We can only append. This gives us a "history," so to speak.
The mutable pointers answer "where am I?" and "what's about to happen?":

- HEAD: points to the current commit. It's how the tool knows where the next commit's parent should be.
- The index: the staging area, path -> blob hash representing the snapshot that `commit` will turn into a tree.

Note: There are refs (pointers) in Git, but this is defaulted to HEAD and a default branch as a minimalist approach.

### Object model

The object model is one of the core tenets of Git. Every object in TinyGit shares the same envelope:

```
<type> <size>\0<content>
```

where size is the byte length of the content, then a NUL terminator, then the content itself.
Below are the 3 types of objects:

1. Blob - A blob object stores raw file contents and nothing else. No filename, no permissions, no timestamps, just raw content. ```blob <size>\0<raw file bytes>```
2. Tree - A tree represents a directory snapshot. The content is a sequence of entries. Entries are sorted by name. Modes are `100644` for a normal file, `100755` for an executable, and `40000`for a subtree (nested directory). ```<mode> <name>\0<20-byte raw hash>``` (For TinyGit, we only use mode 100644 for simplicity)
3. Commit - A commit ties the snapshot to history. The content is the text:

```
tree <40-hex hash>
parent <40-hex hash>       (omitted for the very first commit)
author <name> <email> <unix-timestamp> <tz>
committer <name> <email> <unix-timestamp> <tz>

<commit message>
```

As you've probably noticed, the commit object contains a tree hash and a parent (tree) hash that is 40-character hexadecimal and not the 20-byte raw hash in #2. Why?
Well, the commit object's content is text, and it needs to be readable. Raw bytes would jam non-printable junk into the message, and since hexadecimal is printable ASCII, it can sit inline with the rest of the text.

The three object types connect like this — a commit points at a tree (the snapshot) and at its parent commit (the history), and a tree points at blobs and subtrees:

<img width="1365" height="1140" alt="tinygit_object_flow" src="https://github.com/user-attachments/assets/53f84ec9-95d8-4df3-80ad-fde4cf0fcd7c" />

### Content-addressable storage

Objects are hashed using SHA-1 and stored in the .git/objects directory. The hash is the address, meaning identical content always hashes to the same path. So it is stored exactly once.
The name is derived from the content, so any corruption changes the hash and is immediately detectable.
The storage path is sharded in a 2/38 split inside the object directory, meaning that the hash's first two hex characters are the subdirectory and the rest is the filename.

<img width="1676" height="1326" alt="tinygit_content_storage_flow" src="https://github.com/user-attachments/assets/bff245f8-8d1b-48a0-8fb4-b23dc550b99f" />

### Refs and the commit graph

Refs are files that contain a hash. In TinyGit, ```.git/refs/heads/master``` is a text file whose entire contents are a 40-character commit hash plus a newline. Literally, the branch ```master``` (in TinyGit's case the only branch) points at commit X, which means the file ```refs/heads/master``` contains X's hash. 

```.git/HEAD``` doesn't contain a commit hash, it contains ```ref: refs/heads/master```. So to find the current commit, we need to hop twice: HEAD names a ref, the ref names a commit. This is exactly why we can advance the branch without directly touching HEAD. All we have to do is overwrite the ref file with the new commit's hash, and HEAD continues to point at the same branch.

Note: when there are no commits (like when you initialize tinygit), ```refs/heads/master``` does not exist. Also, before writing a new commit, TinyGit reads the current ref to get the parent hash (the previous commit's hash). If the file exists, that hash becomes the parent line, the commit is a root commit, and ```ref/heads/master``` is created.

<img width="1814" height="746" alt="tinygit_ref_commit_flow" src="https://github.com/user-attachments/assets/33b49662-7cfd-43fc-bba6-9afa373d8f37" />

### The index/staging area

The index is a single binary file ```.git/index```that holds the staging area. The staging area holds the exact set of files that will go into the next commit. It sits directly between the working directory (files you're editing) and the object store (permanent committed objects).

```tinygit add``` writes the blob to the object store immediately and records the file in the index. Example: ```tinygit add hello.txt```; the second this command runs, the blob is already on disk in ```.git/objects``` before any commit exists.

For each staged file (after running tinygit add), there is an entry containing: the file's ```stat``` metadata (think timestamps, inode, uid/gid, size, device, mode), the file's blob hash, and its path. THE ENTRIES ARE SORTED BY THEIR PATH INSIDE THE INDEX. The staged file has a 12-byte header (DIRC signature, format version, entry count) and ends with a SHA-1 checksum over everything preceding it, so corruption is detectable.

## A Quirk and how I made TinyGit's index byte-identical to Git's

In the entry format stated above, each entry  is padded with 1 - 8 NUL bytes so its total length is a multiple of 8, and the path's NUL terminator counts as the first padding byte. Multi-byte fields are stored big-endian regardless of host architecture (via ```htonl/ntohl```). This is what makes TinyGit's index byte-identical to real Git's index.

```
 working dir              index (staging)           object store
┌──────────────┐        ┌──────────────┐         ┌──────────────┐
│ files you've │  add   │ snapshot of  │ commit  │ permanent,   │
│ edited       │ ─────► │ what goes in │ ──────► │ hashed       │
│ (uncommitted)│        │ next commit  │         │ objects      │
└──────────────┘        └──────────────┘         └──────────────┘
```

## Build

```bash
git clone https://github.com/braxtontillman/tinygit.git
cd tinygit
make
sudo make install          # installs to /usr/local/bin
```

For a no-sudo install:

```bash
make install PREFIX=~/.local
```

**Requirements:** a C11 compiler (clang or gcc), OpenSSL, and zlib.

- macOS: `brew install openssl`
- Debian/Ubuntu: `sudo apt install libssl-dev zlib1g-dev`

Run the test suite with:

```bash
make test
```

## Demo TODO

[A short, copy-pasteable walkthrough showing a real session end to end.]

```bash
tinygit init
tinygit add hello.txt
tinygit commit -m "first commit"
tinygit log
```

## Verification TODO

Because TinyGit writes canonical Git objects, you can verify its
output with stock Git plumbing:

    tinygit init
    tinygit add hello.txt
    tinygit commit -m "first commit"

    # inspect TinyGit's objects with REAL git:
    git cat-file -t <hash>     # → blob / tree / commit
    git cat-file -p <hash>     # pretty-prints the object
    git ls-tree <tree-hash>    # lists tree entries
    git log                    # walks the history TinyGit wrote

If real Git can read it, the format is correct.

## Project structure TODO

[A tree of the repo so a reviewer can navigate it. Keep it to the meaningful files.]

```
tinygit/
├── src/
│   ├── [object.cpp]      # [object serialization + hashing]
│   ├── [index.cpp]       # [staging area]
│   └── [...]
├── include/
├── tests/
└── README.md
```

## Design decisions

From the start, I had made design decisions around learning Git intimately and getting better at C. If I viewed something not aligning with these goals, then I deliberately cut it from the project. I refused to scope creep beyond the object model because it takes away from the intricacies of Git and its original purpose as a learning project.

- **[Hashing choice]** — I chose SHA-1 because this is what makes TinyGit byte-compatible with real Git. If I went with SHA-256, then the hash output would be different and defeats one of the main purposes of the project. Note: Git is migrating to SHA-256 in the future, but as of right now it's SHA-1.
- **[Storage format]** — Git stores objects two ways: "loose" (one zlib-compressed file per object, the 2/38 sharded layout mentioned earlier) and "packed"(many objects delta-compressed into a single packfile). I chose just to do loose because they're dead simple. All I had to do was write it and verify it. It's one object, one file, hash it, and done. Where packfiles add delta encoding  and an index format that would've been a project on their own.
- **[Scope cuts]** — I deliberately left out branching, subdirectories, packfiles, and merge. The reason for this was due to the learning curve flattening with these topics as we start to go into areas that are more engineering work than it is learning. The scope was the object model, the conceptual core of Git, not recursive trees and bookkeeping that would end up as busy work.
  
## What I learned

Two things stuck with me most: byte-level discipline, and the gap between understanding and fluency.

The byte-level discipline came from the index formatting. Their index formatting contained packed binary with zero guardrails, where a single wrong byte silently corrupts everything. My NUL padding was off by one in a way that only surfaced for certain filename lengths, and finding bugs like that forced me to develop a real diagnostic loop: dump the bytes, diff them against real Git, and hunt down the mismatch. That loop turned out to be the actual skill, more than getting it right the first time.

The second lesson was subtler. I could reason through every line of this project and explain why it worked, but there were times when I would just stare at a blank file and not know where to begin. I've been working on decomposing the problem more into smaller chunks and taking it one bite at a time.

## Limitations & roadmap

NOTE: These limitations were intended to keep the project going. Most of these are QOL updates or small implementations to make TinyGit even closer to real Git, but I felt as if it was redundant and took away from the core learning.

Here are TinyGit's limitations:

- **No subdirectories.** `write_tree` does not recurse; trees are always flat. Adding a file inside a directory is not supported.
- **Mode is always `100644`.** No executable bits, no symlinks, no submodules.
- **Author identity is hardcoded.** No `.git/config` reading.
- **Timezone is hardcoded to `+0000`.** Commits are valid but always UTC-labeled.
- **Commit messages are capped** by a fixed 4 KB buffer; longer messages are truncated.
- **Objects are loose only.** No packfiles, no delta compression.
- **SHA-1 via a deprecated OpenSSL API.** Functional, but the `EVP_*` migration is outstanding.
- **No `status`, no branching, no merge, no diff.**

Here are some possible next steps:

- [ ] Recursive tree building (subdirectory support)
- [ ] `cat-file` — the read path is already built for `log`
- [ ] `status` — working dir vs index vs HEAD comparison
- [ ] Branching and merge

## AI Usage

This project was written entirely by me, and me alone. AI was used in the learning process, research, and code review. All code and design were written and designed by me.

## TODO: Update all designs with lucid charts before committing
