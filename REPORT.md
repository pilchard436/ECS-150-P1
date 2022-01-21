# sshell$

## Summary:

This program `sshell$`...

##  Implementation:

The implmentation of this prorgam follows 9 distinct steps:

0. Setting up the make file and understand the code for system()
1. Program the fork()+exec()+wait() method
2. Execute command lines containing programs and their arguments
3. Implement builtin commands; pwd, cd
4. Parsing the output and redirecting it to a running specified program
5. Run commands concurrently to allow data to be transmitted from one program
to the next
6. NEEDS SLS INFO
<!-- Stills needs sls -->
7. Redirect standard error specified by two meta-characters to a pipeline or
specified  file
8. Handle errors if incorrect command line is supplied by user

### Phase 1: `fork()+exec()+wait()`

<!-- Fork might need a little bit more of fine tuneing -->

The beginning of this process the shell `fork()` and creates a child process.
The child process then runs the specified command with `execvp()` while the
parent process waits using `waitpid()` until the child process has completed
and the parent is able collect the exit status which is returned by `retval`.

### Phase 2: `Command arguments`

At the beginning of the commands execution we are parsing the command and its
arguments, sperating the string using `strtok()`. This process seperates the
string when specified special characters are found `\\," ",""`. A set counter
`argc` is used to track the argument postion and insert it into an array of
arguments `argv[]`. When command line has been parsed, the command `argv[0]` 
is initially checked for any builtins commands followed by its arguments
`argv[1]`. Then it will flow into a `fork()+exec()+wait()` and execute parsed
command into stdout.

### Phase 3: `Builtin commands`

Once the configuration structure for parsing commands is built, it can now 
detect if command `argv[0]` is a builtin command.

Two helper functions have been created to optimize the code flow.

`cd()`: if `argv[0]` is equal to `cd`, then we will exectute the function.
Unlike, `pwd()` this command takes arguments to change the current working
directory of process to the directory specificed by `$PATH$`. if no argument
`!argv[1]` then it will return an out of bound exit code of 255. If valid
argument is passed in then return 0, else return 1.

`pwd()`; similar to the inital step for `cd()`. Once we have enterted the
function we check `argv` for any following arguments that the command line may
have. If there is an `arg` present in `argv[]` besides `argv[0]` this will
return an out of bound exit code of 255. If no other arguments are inserted
then we use a buffer to get current working directory with `getcwd()` and print out to stdout.

### Phase 4: `Output redirection`
We have three pointers to keep track of the filename, delimiter, and array of
arguments from the command. The process checks if there is an output direction
`>` or `>&`, remove the meta-character, then filename, and finally store it.

We begin the ouput redirection by checking for a one of the specific meta
characters `>` or `>&` using `strstr()`. Once an occurance of the first 
character is found we set two flags `stdout_redirect` and `stderr_redirect` to
a 0 or 1. `strstr()` will return the first occurance in the string if any,
is detected.

Below shows how the flags and delimiter are set for each case, to distingish
between operations needed to be done:

`>&`: Indicates standard error and standard output should be redirected. 
If encountered, `stdout_redirect` and `stderr_redirect` are set to `1` and
delimiter is assigned to `>`.

`>`: Indicates that the command right before meta character is to write output
to the specified file instead of the shell's standard output. If encountered,
`stdout_redirect` is set to 1 and delimiter is assigned to >.

Once the flags and delimiter are set, the delimiter is check if empty (NULL)
. If empty this indicates that command has no redirection. Otherwise, we split
the argument into an array of arguments using the `splitStrtoArr()`. Once there
, arguments are seperated and assigned to either cmd or filename. If
stderr_redirect flag is set then we check for next character to gather filename
in its entirety. 

Once filename and command are avaliable, they are ran different edge cases
prior to opening and creating file to make sure that there is indeed a file
or command to perform the process. If one or both are missing, then
`errormessage()` will send error message out stderr. Finally if successful,
then proccess writes and updates newly created file if file is not created yet, then close file.

### Phase 5: `Pipeline commands`

<!-- Information needed here -->

### Phase 6: `Extra feature(s)`
Standard error redirection:
1. Explained in `Phase 4` under `>&`
2. `|&`:
<!-- Information needed here -->

### Phase 7: `Helper functions`
Function `splitStrtoArr()` performs a method that splits a string based on a
delimter using `strsep()`. Checks for any lingering white-spaces by calling
`trimspaces()` then stores split arguments into an array of arguments. 

Function `trimspaces()` trims leading and trailing spaces of a string and
assigns ending element as null terminator if none is present. 

Function `error_message()` performs the parsing error handling. By running
specific edges cases, a switch statement is used to parse the correct error
message onto stderr specific to the condtion it failed to meet.

Function `completed_process()` retrieves the command and exit status `retval` of the
completed proccess and prints the finished statment with exit status onto stderr. 

Function `char_switch()` goes through command and finds occurances of | or &. If found, it performs a swap of characters and switches those for spaces.

Function `occur()` goes through the entirety of the string to find occurances of a specific character and keeps track of amount when detected.

### Authors:
Created by Jeffery Chang and Sergio Santoyo