# Advanced Operating Systems — Assignment 2

This project implements a set of Unix shell-like functionalities using **C++** and **system calls**. It is tested on **macOS** with `g++`.

---

## Build Instructions

```bash
# Clean old builds
make clean

# Build the project
make

# Run the shell
./main
```

## Project Structure
### Core Files

- **main.cpp** — Entry point of the shell, command parsing and execution loop.
    
- **makefile** — Build instructions.
    

### Include (Headers)

- **prompt.h** — Functions to display the shell prompt (`username@hostname cwd %`).
    
- **cd.h** — Implementation of `cd` (change directory).
    
- **pwd_local.h** — Implementation of `pwd` (print working directory).
    
- **echo.h** — Implementation of `echo`.
    
- **ls.h** — Implementation of `ls` (list files/directories).
    
- **pinfo.h** — Implementation of `pinfo` (process info).
    
- **history.h** — Command history tracking.
    
- **signals.h** — Signal handling for foreground/background jobs.
    
- **ioredirection.h** — Handles input/output redirection (`<`, `>`, `>>`, `2>`).
    
- **pipeline.h** — Handles pipelines (`|`).
    
- **systemcommands.h** — Execution of external system commands.
    
- **builtincommands.h** — Dispatching built-in commands.
    
- **utils.h** — Utility functions (string processing, path handling).
    
- **search.h** — File/directory search functionality.
    
- **autocomplete.h** — Autocompletion support (commands + files).
    
- **project.h** — Common project-wide definitions.
    

### Source (Implementations)

- **prompt.cpp** — Implements shell prompt logic using `getuid`, `getpwuid`, `gethostname`, `getcwd`.
    
- **cd.cpp** — Implements `cd` using `chdir`.
    
- **pwd_local.cpp** — Implements `pwd` using `getcwd`.
    
- **echo.cpp** — Implements `echo` command.
    
- **ls.cpp** — Implements `ls` using `opendir`, `readdir`, `stat`.
    
- **pinfo.cpp** — Implements `pinfo` using process-related system calls.
    
- **history.cpp** — Maintains command history in memory and file.
    
- **signals.cpp** — Implements signal handling (`SIGINT`, `SIGTSTP`, `SIGCHLD`).
    
- **ioredirection.cpp** — Implements I/O redirection using `open`, `dup2`, `close`.
    
- **pipeline.cpp** — Implements command pipelines using `pipe`, `fork`, `dup2`.
    
- **systemcommands.cpp** — Executes external commands with `fork` + `execvp`.
    
- **builtincommands.cpp** — Dispatches built-in commands.
    
- **utils.cpp** — Helper methods (string parsers and tokenizers).
    
- **search.cpp** — Implements file searching.
    
- **autocomplete.cpp** — Implements command/file autocompletion.



#### Workflows and System Calls (Q1–Q12)

## Workflows and System Calls

### Q1. Display requirement: prompt and tokenization
**Workflow**
1. Readline or `getline` fetches input. Trim spaces/tabs. Split on `;` then tokenize with `strtok` per spec.  
2. Prompt format: `user@host:cwd>` where shell “home” path is shown as `~`.

**Main calls**
- `getuid` → `getpwuid` for username.
- `gethostname` for system name.
- `getcwd` to print cwd, map `$HOME_SHELL` to `~`.
- Tokenize with `strtok` (as required).  
- Errors via `perror`.

---

### Q2. Built-ins: `cd`, `pwd`, `echo`
**Workflow**
- Dispatch built-ins without `execvp`.
- `cd` handles no arg → home, `.` `..` `-` `~`. Update internal `OLDPWD`.
- `pwd` prints absolute path.
- `echo` prints args verbatim; handle spaces/tabs as text.

**Main calls**
- `chdir` for directory change.
- `getcwd` for `pwd`.
- `write` for `echo`.  

---

### Q3. `ls` with `-a`, `-l`, paths
**Workflow**
1. Parse flags and targets (default `.`). Multiple dirs allowed, any order.
2. For each target:
   - If `-l`: build long listing row per entry.
   - If `-a`: include `.` and `..`.
3. Resolve `~`, `.` `..`, absolute vs relative.

**Main calls**
- `opendir` / `readdir` / `closedir`.
- `lstat` or `stat` for type, perms, links, owner, group, size, mtime.
- `getpwuid` / `getgrgid` to print owner/group.

---

### Q4. System commands in foreground/background (`&`)
**Workflow**
1. If not a built-in, prepare argv and environment.
2. Fork:
   - Child: set process group, restore default signals, `execvp(cmd, argv)`.
   - Parent: 
     - FG: set terminal to child’s PGID, `waitpid` until exit/stop, then reclaim terminal.
     - BG (`&`): do not wait; print PID; track job list.

**Main calls**
- `fork`, `execvp`.
- `setpgid` to form a new process group.
- `waitpid(WUNTRACED|WCONTINUED)` to reap/track.
- `signal`/handlers for `SIGCHLD`.

---

### Q5. `pinfo [pid]`
**Workflow**
- If no PID, use shell’s PID. Query status, memory (VM size), executable path. Mark `+` if in foreground.

**Main calls**
- Portable approach on macOS: `proc_pidinfo` / `proc_pidpath` or `sysctl` for kinfo; on Linux: read `/proc/<pid>/*`.  
- Also `getpgid`, `tcgetpgrp` to determine foreground.  
- Print: Status `R/S/T/Z`, `+` if FG, VM size KB, Exec path with `~` collapse.

---

### Q6. `search <name>` recursive under cwd
**Workflow**
- DFS from cwd. Match file or directory name. Stop early on first match. Print `True`/`False`.

**Main calls**
- `opendir` / `readdir` / `closedir`.
- `lstat` to distinguish dir vs file.

---

### Q7. I/O redirection `<`, `>`, `>>`
**Workflow**
1. Parse redirection operators independent of command tokens.
2. Before `execvp` (or built-in execution), in the child context:
   - Input: `fd = open(in, O_RDONLY)`, `dup2(fd, STDIN_FILENO)`.
   - Output: `open(out, O_CREAT|O_WRONLY|O_TRUNC, 0644)` then `dup2`.
   - Append: `open(out, O_CREAT|O_WRONLY|O_APPEND, 0644)` then `dup2`.
3. Close fds, then run command.

**Main calls**
- `open`, `dup2`, `close`.
- Error if input file missing; create outputs with `0644`.

---

### Q8. Pipelines
**Workflow**
1. Split stages by `|`.
2. Create N−1 pipes.
3. For each stage `i`:
   - `fork`, set PGID for the pipeline.
   - If not first: `dup2(prev_read, STDIN_FILENO)`.
   - If not last: `dup2(pipe[i][1], STDOUT_FILENO)`.
   - Close all pipe fds, `execvp`.
4. Parent closes all pipe fds; waits if FG.

**Main calls**
- `pipe`, `fork`, `dup2`, `close`, `execvp`, `waitpid`.

---

### Q9. Redirection with pipelines
**Workflow**
- Parse redirection per stage. Apply each stage’s `<`, `>`, `>>` in that child after wiring its `dup2` for pipes and before `execvp`.

**Main calls**
- Same as Q7 + Q8 combined: `pipe`, `open`, `dup2`, `close`, `execvp`.

---

### Q10. Simple signals: Ctrl-Z, Ctrl-C, Ctrl-D
**Workflow**
- Install handlers in shell:
  - `SIGTSTP` (Ctrl-Z): if FG job exists, send `SIGTSTP` to its PGID, move to BG list as “stopped”.
  - `SIGINT` (Ctrl-C): send `SIGINT` to FG PGID; shell remains unaffected.
  - `SIGCHLD`: reap zombies; update job states.
- Ctrl-D: detect EOF on input; cleanly exit shell loop.

**Main calls**
- `signal`/`sigaction` for `SIGINT`, `SIGTSTP`, `SIGCHLD`.
- `kill(-pgid, SIGINT/SIGTSTP)`.
- `waitpid(-1, &st, WNOHANG|WUNTRACED|WCONTINUED)`.

---

### Q11. Autocomplete for commands and files
**Workflow**
- Bind TAB to completion. If unique, auto-complete. If multiple, expand common prefix and list matches.
- Complete from:
  - Built-ins list.
  - Files/dirs in cwd.

**Main calls / APIs**
- GNU Readline stack (typical): `rl_bind_key`, `rl_attempted_completion_function`, `add_history`.  
- Filesystem scanning via `opendir`/`readdir` for candidates.

---

### Q12. `history` command and Up/Down navigation
**Workflow**
- Persist last 20 commands across sessions. Show last 10 by default; `history <n>` shows latest `n` (≤20).
- On each accepted line, append to on-disk history and in-memory ring buffer.
- Up/Down traverse history; when at ends, stay; surfaced command is editable.

**Main calls / APIs**
- readline: `read_history`, `write_history`, `add_history`, `previous_history`
