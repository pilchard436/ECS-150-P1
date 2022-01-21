# sshell$

## Summary:

This program `sshell$` was aimed to help us understand UNIX system call
through the development of our own shell. A shell is a command-line 
interpreter that accpets user input in the form of command lines and 
executes them. `sshell` will perform a set of core processes; 
Execute, Selection, Redirection, and Composition.

##  Implementation:

The implmentation of this prorgam follows 9 distinct steps:

0. Setting up the make file and understand the code for system()
1. Program the fork()+exec()+wait() method
2. Execute command lines containing programs and their arguments
3. Implement builtin commands; pwd, cd
4. Parsing the output and redirecting it to a running specified program
5. Run commands concurrently to allow data to be transmitted from one program
to the next
6. Implement sls retrieves current working directory and prints file along
with its size.
7. Redirect standard error specified by two meta-characters to a pipeline or
specified  file
8. Handle errors if incorrect command line is supplied by user

### Phase 1: `fork()+exec()+wait()`

The beginning of this process the shell `fork()` and creates a child process.
The child process then runs the specified command with `execvp()` while the
parent process waits using `waitpid()` until the child process has completed
and the parent is able collect the exit status which is returned by `retval`.

### Phase 2: `Command arguments`

At the beginning of the commands execution we are parsing the command and its
arguments, sperating the string using `strsep()`. This process seperates the
string when specified characters are found. A set counter `argc` is used to 
track the argument postion and insert it into an array of
arguments `argv[]`. When command line has been parsed, the command `argv[0]` 
is initially checked for any builtins commands followed by its arguments
`argv[1]`. Then it will flow into a `fork()+exec()+wait()` and execute parsed
command into `stdout`.

### Phase 3: `Builtin commands`

Once the configuration structure for parsing commands is built, it can now 
detect if command `argv[0]` is a builtin command.

Two helper functions have been created to optimize the code flow.

`cd()`: if `argv[0]` is equal to `cd`, then we will exectute the function.
Unlike `pwd()`, this command takes arguments to change the current working
directory of process to the directory specificed by `$PATH$`. if no argument
`!argv[1]` then it will return an out of bound exit code of 255. If valid
argument is passed in then return 0, else return 1.

`pwd()`; similar to the inital step for `cd()`. Once we have enterted the
function we check `argv` for any following arguments that the command line may
have. If there is an `arg` present in `argv[]` besides `argv[0]` this will
return an out of bound exit code of 255. If no other arguments are inserted
then we use a buffer to get current working directory with `getcwd()` 
and print out to `stdout`.

### Phase 4: `Output redirection`
We have three pointers to keep track of the filename, delimiter, and array of
arguments from the command. The process checks if there is an output direction
`>` or `>&`, remove the meta-character, then filename, and finally store it.

We begin the output redirection by checking for a one of the specific meta
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
`stdout_redirect` is set to 1 and delimiter is assigned to `>`.

Once the flags and delimiter are set, the delimiter is checked if empty (NULL)
. If empty this indicates that command has no redirection. Otherwise, we split
the argument into an array of arguments using the `splitStrtoArr()`. Once there
, arguments are seperated and assigned to either cmd or filename. If
`stderr_redirect` flag is set then we check for next character to gather 
filename in its entirety. 

Once filename and command are avaliable, they are ran through different edge
cases prior to opening and creating file to make sure that there is indeed a
file or command to perform the process. If one or both are missing, then
`errormessage()` will send error message out to `stderr`. Then the shell checks
if a file is able to be opened or created. Finally if successful, we continue
with execution. 

### Phase 5: `Pipeline commands`

If we have more than 1 command, then for each additional process, we need 
to fork in order to execute them concurrently. We also need to pipe the
previous process's `stdout` and/or `stderr` into the next process's `stdin`.

First we create a duplicate of the existing `stdout`, `stderr`, and `stdin` so
we can restore them to their initial state when we are done. Then we go into
a for loop, and for each process, we create a pipe and `fork()`, then in the
parent we wait for the child to finish, then we copy the current `stdin` into
our newly created pipe's read end. Next, we check for the exit code returned
by the child, perform any error handling, then proceed to the next iteration of
the for loop. For the child, we need to change the `stdin` to the previous
pipe's read end. Then depending on the total number of processes, which process
is currently executing, and whether we have an output redirection. We redirect 
`stdout` and/or `stderr` to either write to the pipe, write onto the terminal,
or write into the provided file. Then we execute the command and exit. 

Finally, after all the processes are executed, we restore `stdin`, 
`stdout` and `stderr`, and print out the `+ completed ...` message using 
`completed_process()`. 

### Phase 6: `Extra feature(s)`
Standard error redirection:
1. Explained in `Phase 4` under `>&`
2. `|&`: we first check whether we need to pipe error. We do this by checking 
whether there is a `&` after `|`. Then when we need to pipe error, we 
change `stderr` to write to the pipe together with `stdout`. 
3. `sls` (simple ls): We create object for dp and sb which is our data
path and size signal. We retrieve our currently working directory using
`opendir(.)`. Once the directory is present, we read each file in directory
and as long as it is not a hidden file. We print the file and file size to
`stdout`.

### Phase 7: `Helper functions`

Function `splitStrtoArr()` performs a method that splits a string based on a
delimter using `strsep()`. Checks for any lingering white-spaces by calling
`trimspaces()` then stores split arguments into an array of arguments. 

Function `trimspaces()` trims leading and trailing spaces of a string by 
doing some pointer arithmatics and
assigns ending element as null terminator if none is present. 

Function `error_message()` performs the parsing error handling. By running
specific edges cases, a switch statement is used to parse the correct error
message onto `stderr` specific to the condtion it failed to meet.

Function `completed_process()` retrieves the command and exit status `retval` 
of the completed proccess and prints the finished statment with exit status 
onto `stderr`. 

Function `char_switch()` goes through command and finds occurances of `|` or 
`&`. If found, it performs a swap of characters and switches those for spaces.

Function `occur()` goes through the entirety of the string to find occurances 
of a specific character and keeps track of amount when detected.

Function `execute_process()` first check if the process being asked to execute 
is a built-in function. Otherwise, it forks itself and the parent waits for 
the child to return an exit status. The child execute the command. 

## References:
1. Split string into array:  https://c-for-dummies.com/blog/?p=1769
2. Trimming whitespace: https://stackoverflow.com/questions/122616/
how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
3. Pipe implementation: https://stackoverflow.com/questions/60804552/
pipe-two-or-more-shell-commands-in-c-using-a-loop
4. forking and waiting: https://stackoverflow.com/questions/13654315/two-
forks-and-the-use-of-wait
5. Ignoring hidden files: https://stackoverflow.com/questions/845556/
how-to-ignore-hidden-files-with-opendir-and-readdir-in-c-library
6. CWD info: https://stackoverflow.com/questions/59983516/
what-does-signify-in-c-as-argument-to-opendir
7. GNUC Libary: https://www.gnu.org/software/libc/manual/html_mono/libc.
html#Running-a-Command

### Authors:
Created by Jeffery Zhang and Sergio Santoyo