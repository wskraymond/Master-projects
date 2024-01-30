
ParallelBF(java) 
*A program used to find a shortest path with a network with billions of nodes (Big Data problem)

*Using distributed platform (Hadoop) , by its map-reduce programming & parallel breath-first search algo

*Infrastructure: (IaaS)4 VM (one master node, others are slave nodes) of open stack platform (Open Source Cloud Computing Software)

*Distributed Computing Platform: Hadoop set up on 4 VM (can be more)

Related to the following tech:
1. hadoop(Distributed computing)
2. Map-reduce programming in hadoop , deal with big data problem
3. Parallel Breadth first search in Map-reduce
__________________________________________________________________________________________


cloud storage with de-duplication (java) 
*A dropbox-like cloud storage program with de-duplication algo
*using window azure cloud storage service

Related to the following tech
1.De-duplication algorithm to minimize the actual total amount of files saved into storage
2 cloud storage
__________________________________________________________________________________________

Proxy Server (C language)
*HTTP Proxy Server simplified implemetation

Related to the following tech
1.socket programming
2.multi-threading , Synchronization
3.Implementation of Http protocol manipulation for proxy server


__________________________________________________________________________________________
WSL Installation and Setup
https://learn.microsoft.com/en-us/windows/wsl/install#prerequisites
https://learn.microsoft.com/en-us/windows/wsl/setup/environment#set-up-your-linux-username-and-password
https://superuser.com/questions/1566022/how-to-set-default-user-for-manually-installed-wsl-distro
https://code.visualstudio.com/docs/remote/troubleshooting#_fixing-problems-with-the-code-command-not-working


Why WSL?
WSL lets you run a Linux environment -- including command-line tools and applications -- directly on Windows, 
without the overhead of a traditional virtual machine or dualboot setup. 
WSL especially helps web developers and those working with Bash and Linux-first tools 
(i.e. Ruby, Python) to use their toolchain on Windows and ensure consistency between development and production environments.

Why the WSL extension in VS Code?
With VS Code and the WSL extension combined, VS Code’s UI runs on Windows, 
and all your commands, extensions, and even the terminal, run on Linux. 
You get the full VS Code experience, including autocomplete and debugging, 
powered by the tools and compilers installed on Linux.

Getting started
You can launch a new instance of VS Code connected to WSL by opening a WSL terminal, navigating to the folder of your choice, and typing code .:

To get started with your first app using the WSL extension, check out the step-by-step WSL tutorial in docs:
https://code.visualstudio.com/docs/remote/wsl-tutorial