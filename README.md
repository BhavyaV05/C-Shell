# C-Shell

A custom Unix shell implementation written in C, featuring built-in commands, job control, I/O redirection, and pipeline execution.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Built-in Commands](#built-in-commands)
- [Advanced Features](#advanced-features)
- [Technical Details](#technical-details)
- [Examples](#examples)

## Overview

C-Shell is a command-line interpreter that provides an interactive interface for executing commands, managing processes, and navigating the file system. It implements a subset of common shell features found in popular Unix shells like bash and zsh, with custom built-in commands for enhanced functionality.

The shell displays a custom prompt showing the current user, hostname, and working directory, making it easy to track your location in the file system.

## Features

### Core Functionality
- **Custom prompt**: Displays `<username@hostname:current_directory>` with tilde expansion for home directory
- **Command execution**: Run system commands and executables
- **Input parsing**: Robust tokenization and syntax validation
- **Signal handling**: Proper handling of `Ctrl-C` (SIGINT) and `Ctrl-Z` (SIGTSTP)
- **EOF handling**: Exit cleanly with `Ctrl-D`

### Built-in Commands
- **hop**: Navigate directories with special path handling
- **reveal**: List directory contents with various display options
- **activities**: View all background and stopped processes
- **ping**: Send signals to processes by PID
- **fg**: Bring background/stopped jobs to foreground
- **bg**: Resume stopped jobs in background

### Process Management
- **Background execution**: Run commands in background using `&`
- **Job control**: Track, manage, and control background and stopped processes
- **Process groups**: Proper process group management for job control
- **Signal forwarding**: Forward signals to foreground process groups

### I/O Operations
- **Input redirection** (`<`): Redirect stdin from a file
- **Output redirection** (`>`): Redirect stdout to a file (overwrite)
- **Append redirection** (`>>`): Redirect stdout to a file (append)
- **Pipeline support** (`|`): Chain commands with pipes
- **Command chaining** (`;`): Execute multiple commands sequentially

## Installation

### Prerequisites
- GCC compiler (with C99 support)
- POSIX-compliant operating system (Linux, macOS, or Unix-like)
- Make utility

### Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/BhavyaV05/C-Shell.git
cd C-Shell
```

2. Navigate to the shell directory:
```bash
cd shell
```

3. Compile the source files manually:
```bash
gcc -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
    -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm -g \
    -I./include -c src/*.c

gcc *.o -o shell.out
```

4. Run the shell:
```bash
./shell.out
```

### Clean Build Artifacts

To clean up compiled object files and executables:
```bash
rm -f *.o shell.out
```

## Usage

After launching the shell with `./shell.out`, you'll see a prompt like:
```
<username@hostname:~>
```

From here, you can execute commands just like in any Unix shell.

### Exiting the Shell
- Type `exit` and press Enter
- Press `Ctrl-D` (EOF)

## Built-in Commands

### hop - Change Directory

Navigate through the file system with advanced path handling.

**Syntax:**
```bash
hop [directory...]
```

**Features:**
- `hop` - Navigate to home directory
- `hop <path>` - Navigate to specified directory
- `hop ~` - Navigate to home directory
- `hop -` - Navigate to previous directory
- `hop ..` - Navigate to parent directory
- `hop .` - Stay in current directory
- Multiple arguments: Navigate through multiple directories in sequence

**Examples:**
```bash
hop                    # Go to home directory
hop /usr/local/bin     # Go to /usr/local/bin
hop ..                 # Go to parent directory
hop -                  # Go to previous directory
hop ~/Documents        # Go to Documents in home directory
hop dir1 dir2 dir3     # Navigate to dir1, then dir2, then dir3
```

### reveal - List Directory Contents

Display files and directories with customizable formatting.

**Syntax:**
```bash
reveal [flags] [directory]
```

**Flags:**
- `-a` - Show hidden files (files starting with `.`)
- `-l` - Display in line-by-line format (one entry per line)
- Flags can be combined: `-al` or `-la`

**Features:**
- Lexicographic sorting of entries
- Supports all path formats (relative, absolute, `~`, `.`, `..`, `-`)
- Default format shows entries separated by spaces

**Examples:**
```bash
reveal                 # List current directory
reveal -a              # List current directory including hidden files
reveal -l              # List current directory, one per line
reveal -al /etc        # List /etc with hidden files, one per line
reveal ~               # List home directory
reveal -la ..          # List parent directory with all files, one per line
```

### activities - View Process List

Display all background and stopped processes managed by the shell.

**Syntax:**
```bash
activities
```

**Output Format:**
```
[PID] : command_name - State
```

**Features:**
- Lists all active background and stopped jobs
- Sorted alphabetically by command name
- Shows process state (Running or Stopped)
- Displays actual process PID (not job ID)

**Example Output:**
```bash
<user@host:~> activities
[1234] : sleep - Running
[1235] : vim - Stopped
[1236] : find - Running
```

### ping - Send Signals to Processes

Send signals to processes by PID.

**Syntax:**
```bash
ping <pid> <signal_number>
```

**Features:**
- Signal number is taken modulo 32
- Works with any valid process PID
- Provides feedback on signal delivery success

**Common Signal Numbers:**
- `9` - SIGKILL (terminate immediately)
- `15` - SIGTERM (terminate gracefully)
- `19` - SIGSTOP (stop/pause process)
- `18` - SIGCONT (continue stopped process)

**Examples:**
```bash
ping 1234 9            # Kill process 1234
ping 5678 15           # Terminate process 5678 gracefully
ping 9012 19           # Stop process 9012
ping 3456 50           # Send signal (50 % 32 = 18, SIGCONT)
```

### fg - Foreground Job

Bring a background or stopped job to the foreground.

**Syntax:**
```bash
fg [job_id]
```

**Features:**
- Without argument: brings most recent job to foreground
- With job_id: brings specified job to foreground
- Automatically resumes stopped jobs
- Gives terminal control to the job

**Examples:**
```bash
fg                     # Bring most recent job to foreground
fg 2                   # Bring job [2] to foreground
```

### bg - Background Job

Resume a stopped job in the background.

**Syntax:**
```bash
bg [job_id]
```

**Features:**
- Without argument: resumes most recent stopped job
- With job_id: resumes specified stopped job
- Job continues running in background

**Examples:**
```bash
bg                     # Resume most recent stopped job in background
bg 1                   # Resume job [1] in background
```

## Advanced Features

### Background Execution

Run commands in the background by appending `&`:

```bash
sleep 100 &            # Run sleep in background
find / -name "*.txt" & # Search in background
```

When a background job starts, the shell displays:
```
[job_id] pid
```

When a background job completes, the shell displays (before next prompt):
```
[job_id] Done - command_name
```

### I/O Redirection

**Input Redirection:**
```bash
wc -l < input.txt                    # Read from input.txt
sort < unsorted.txt                  # Sort lines from file
```

**Output Redirection:**
```bash
ls -la > output.txt                  # Overwrite output.txt
echo "Hello" > greeting.txt          # Create/overwrite file
```

**Append Redirection:**
```bash
echo "New line" >> file.txt          # Append to file.txt
date >> log.txt                      # Append date to log
```

**Combined Redirection:**
```bash
sort < input.txt > sorted.txt        # Input from file, output to file
grep "error" < log.txt >> errors.txt # Search and append results
```

### Pipelines

Chain multiple commands together:

```bash
ls -la | grep ".txt"                         # List and filter
cat file.txt | sort | uniq                   # Process file content
ps aux | grep python | wc -l                 # Count Python processes
find . -name "*.c" | xargs wc -l | sort -n   # Count lines in C files
```

**Pipeline with Built-in Commands:**
```bash
reveal -a | grep "test"              # List and filter files
```

### Command Sequences

Execute multiple commands with semicolons:

```bash
hop /tmp ; ls -la ; hop ~            # Navigate and list
echo "Start" ; sleep 5 ; echo "End"  # Sequential execution
```

### Signal Handling

**Ctrl-C (SIGINT):**
- Terminates foreground process
- Shell remains running
- Does not terminate the shell itself

**Ctrl-Z (SIGTSTP):**
- Stops (pauses) foreground process
- Adds process to job list
- Returns control to shell
- Process can be resumed with `fg` or `bg`

**Example Session:**
```bash
<user@host:~> sleep 100
^Z
[1] Stopped sleep
<user@host:~> activities
[12345] : sleep - Stopped
<user@host:~> bg 1
<user@host:~> activities
[12345] : sleep - Running
```

## Technical Details

### Architecture

The shell is organized into several modules:

- **main.c**: Main loop, initialization, and command dispatch
- **prompt.c**: Prompt generation and home directory tracking
- **parser.c**: Input tokenization and syntax validation
- **functs.c**: Built-in command implementations
- **jobs.c**: Job control and process management
- **pipes.c**: Pipeline and I/O redirection handling

### Compilation Flags

The project uses strict compilation settings:
- **Standard**: C99
- **POSIX compliance**: `_POSIX_C_SOURCE=200809L`, `_XOPEN_SOURCE=700`
- **Warnings**: `-Wall -Wextra -Werror`
- **Debugging**: `-g` flag included

### Process Management

- Each external command runs in its own process group
- Background processes have stdin redirected to `/dev/null`
- Proper signal forwarding to foreground process groups
- Job state tracking (Running, Stopped, Terminated)
- Maximum 100 concurrent jobs supported

### Memory Management

- Static buffers for path handling (4096 bytes)
- Maximum 64 tokens per command line
- Maximum 32 commands per pipeline
- Proper cleanup and memory deallocation

## Examples

### Basic Navigation
```bash
<user@host:~> hop /usr/local
<user@host:/usr/local> reveal -la
.  ..  bin  etc  include  lib  share
<user@host:/usr/local> hop bin
<user@host:/usr/local/bin> hop -
<user@host:/usr/local> hop ~
<user@host:~>
```

### Background Jobs and Job Control
```bash
<user@host:~> sleep 100 &
[1] 12345
<user@host:~> sleep 200 &
[2] 12346
<user@host:~> activities
[12345] : sleep - Running
[12346] : sleep - Running
<user@host:~> ping 12345 19
Sent signal 19 to process with pid 12345
<user@host:~> activities
[12345] : sleep - Stopped
[12346] : sleep - Running
<user@host:~> fg 1
# (sleep continues in foreground)
^Z
[3] Stopped sleep
<user@host:~> bg
<user@host:~> activities
[12345] : sleep - Running
[12346] : sleep - Running
```

### Complex Pipelines
```bash
<user@host:~> cat data.txt | grep "important" | sort | uniq > results.txt
<user@host:~> reveal -la /etc | grep ".conf" | wc -l
42
<user@host:~> ps aux | grep myprocess | awk '{print $2}' | xargs ping 9
```

### Command Chaining
```bash
<user@host:~> hop /tmp ; reveal -l ; echo "Done" > status.txt ; hop ~
```

### Mixed Operations
```bash
<user@host:~> (sleep 5 ; echo "Finished") &
[1] 12347
<user@host:~> find . -name "*.c" > c_files.txt &
[2] 12348
<user@host:~> activities
[12347] : sleep - Running
[12348] : find - Running
```

## Error Handling

The shell provides clear error messages for various conditions:

- **Invalid syntax**: `INVALID SYNTAX`
- **Directory not found**: `No such directory!`
- **Command not found**: `command: command not found`
- **Invalid flags**: `reveal: Invalid flag -x`
- **Invalid PID**: `Invalid PID: abc`
- **Process not found**: `No such process found`
- **No jobs**: `No jobs to bring to foreground`

## Limitations

- Maximum 100 concurrent jobs
- Path length limited to 4096 characters
- Input line limited to 1024 characters
- Maximum 64 tokens per command
- Cannot handle shell built-ins in background directly (use workarounds)

## Contributing

This is an educational project demonstrating shell implementation concepts. Contributions are welcome for bug fixes and feature enhancements.

## Author

Created by [BhavyaV05](https://github.com/BhavyaV05)

## License

This project is available for educational purposes.
