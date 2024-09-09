
## Let's re-invent the wheel

#### ParallelBF(java) 
A program used to find a shortest path with a network with billions of nodes (Big Data problem) using distributed platform (Hadoop) , by its map-reduce programming & parallel breath-first search algo

> Infrastructure:
  - (IaaS)4 VM (one master node, others are slave nodes) of open stack platform (Open Source Cloud Computing Software)

> Distributed Computing Platform:
  - Hadoop set up on 4 VM (can be more)

> Related to the following tech:
 - hadoop(Distributed computing)
 - Map-reduce programming in hadoop , deal with big data problem
 - Parallel Breadth first search in Map-reduce

#### Cloud storage with de-duplication (java) 
A dropbox-like cloud storage program with de-duplication algo using window azure cloud storage service

> Related to the following tech
 - De-duplication algorithm to minimize the actual total amount of files saved into storage
 - cloud storage

#### Proxy Server (C language)
HTTP Proxy Server simplified implemetation

> Related to the following tech
- socket programming
- multi-threading , Synchronization
- I/O Multiplexing
- buffering for bulk file download
- Implementation of Http protocol manipulation for proxy server

#### Bash shell (C language)
A shell implements many important aspects of the linux operationg system in terms of your daily user , including process management and I/O redirection.

> Related to the following tech
- process management
- signal handling
- Pipe , I/O redirection

## Development Env

#### Overview
https://code.visualstudio.com/docs/remote/remote-overview
https://code.visualstudio.com/docs/editor/command-line

- GCC stands for GNU Compiler Collection; 
- GDB is the GNU debugger.
- WSL is a Linux environment within Windows that runs directly on the machine hardware,
 not in a virtual machine.

> Visual Studio Code has support for working directly in WSL with the WSL extension. recommend this mode of WSL development, where all your source code files, in addition to the compiler, are hosted on the Linux distro.

> WSL Installation and Setup
- https://learn.microsoft.com/en-us/windows/wsl/install#prerequisites
- https://learn.microsoft.com/en-us/windows/wsl/setup/environment#set-up-your-linux-username-and-password
- https://superuser.com/questions/1566022/how-to-set-default-user-for-manually-installed-wsl-distro
- https://code.visualstudio.com/docs/remote/troubleshooting#_fixing-problems-with-the-code-command-not-working

#### Why WSL?
1) WSL lets you run a Linux environment -- including command-line tools and applications -- directly on Windows, without the overhead of a traditional virtual machine or dualboot setup. 
2) WSL especially helps web developers and those working with Bash and Linux-first tools
(i.e. Ruby, Python) to use their toolchain on Windows and ensure consistency between development and production environments.

#### Why the WSL extension in VS Code?
1) With VS Code and the WSL extension combined, VS Code’s UI runs on Windows, and all your commands, extensions, and even the terminal, run on Linux. 
2) You get the full VS Code experience, including autocomplete and debugging, powered by the tools and compilers installed on Linux.

> Getting started
- You can launch a new instance of VS Code connected to WSL by opening a WSL terminal, navigating to the folder of your choice, and typing

```bash
  code .
```

> To get started with your first app using the WSL extension, check out the step-by-step WSL tutorial in docs:
- https://code.visualstudio.com/docs/remote/wsl-tutorial
- https://phoenixnap.com/kb/how-to-create-sudo-user-on-ubuntu

```bash
wsl --update

wsl -shutdown

wsl -d ubuntu -u <usrname>
```

## VS Project Setup
https://code.visualstudio.com/docs/cpp/config-wsl

> If you click on the Remote Status bar item, you will see a dropdown of Remote commands appropriate for the session. 
 - For example, if you want to end your session running in WSL, you can select the Close Remote Connection command from the dropdown. 
 - Running code . from your WSL command prompt will restart VS Code running in WSL.
 - The code . command opened VS Code in the current working folder, which becomes your "workspace". As you go through the tutorial, you will see three files created in a .vscode folder in the workspace:
    1) c_cpp_properties.json (compiler path and IntelliSense settings)
    2) tasks.json (build instructions)
    3) launch.json (debugger settings)


## Build cmd for C program
- https://marketplace.visualstudio.com/items?itemName=ms-vscode.makefile-tools

> make -f Makefile 
