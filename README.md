# Diff Patch Utility


## Introduction

The Diff Patch Utility is a command-line program designed to apply patches to files based on a provided diff file. It offers various options for customizing the patching process, making it a versatile tool for managing changes in text files.

This utility is part of a larger project and is implemented in C.

## Usage

To use the Diff Patch Utility, follow the steps below:

1. **Compilation**: First, compile the program using a C compiler (e.g., GCC). Refer to the [Compilation](#compilation) section for details.

2. **Command-Line Arguments**: Provide the necessary command-line arguments to specify the operation and options. Details on supported arguments can be found in the [Command-Line Arguments](#command-line-arguments) section.

3. **Patch Operation**: The utility will apply the patch to the input file based on the provided diff file and selected options.

4. **Result**: Depending on the success of the operation, the utility will return an appropriate exit code. A return code of 0 typically indicates a successful operation.

## Compilation

To compile the Diff Patch Utility, you can use a C compiler like GCC. Here's an example of the compilation process:

```bash
gcc -o diffpatch main.c fliki.c global.c debug.c
This command assumes that the source files `main.c`, `fliki.c`, `global.c`, and `debug.c` are present in the current directory. Adjust the compiler flags and source file paths as needed.

## Command-Line Arguments
The Diff Patch Utility supports the following command-line arguments:

- `-h`: Display the help message, which provides information about how to use the utility and its options.

- `-n`: Enable numeric mode. This option may affect the patching behavior in a specific way, depending on the utility's implementation.

- `-q`: Enable quiet mode. In this mode, the utility may suppress some or all output messages, depending on its implementation.

The utility expects the diff filename to be provided as the last argument. Ensure that the diff file exists and is accessible.

