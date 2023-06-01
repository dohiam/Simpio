# Simpio Structure, Design, and Dependencies

## Major Components

Simpio is composed of the following major subsystems:

1. Parser
2. Execution Engine
3. User Interface (UI)
4. Main Program

The first 3 subsystems exist mostly independently of each other. There is some dependency between the Parser and Execution Engine because the Parser also generates  instructions and performs hardware configuration, and it uses some functions from the execution engine to do this. One other area of interdependency is status messages that are generated during execution, so the execution engine uses a status message function from the user interface. 

Simpio currently uses static allocation only, i.e., it uses fixed size arrays. This isn't too unreasonable since PIO programs are very small, but needs to be added in the future for more efficient use of memory as well as possibly larger or more complex programs. 

## Theory of Operation

Interdependency between the first three subsystems was maximized so that each of these three pieces could be replaced with a different implementation. For example the current UI uses Ncurses, but one might rather have a true graphical user interface and so might want to replace the UI subsystem with something completely different. To help support replacing the UI, the Main program provides the UI with a set of callbacks to perform building, stepping, running, setting breakpoints, etc. This allows the UI to be independent of the execution engine and parser by calling functions provided by Main, which then call functions from the Parser and Execution Engine to do the work. 

The main processing flow is:

1. The Main program first brings up the UI, and provides to it a set of callbacks to perform each of the PF function key operations. 
2. In this state, before the input program has been parsed, the only valid operations are to edit the program, save it, and initiate building the program. 
3. The Parser reads the input file and both checks for valid instruction syntax and for each instruction makes calls to the execution engine to configure hardware values and generate internal data structures that represent the instructions. Rather than using the same binary representation that real PIO programming uses, Simpio uses a set of data structures that make it easier to simulate running the instructions.
4. Once the input PIO program has been parsed successfully, Then other actions to run, step, and set breakpoints can be used. Each of these makes a callback to Main which in turn calls the execution engine to do the work. After each bit of execution, Main also provides all the data show in the upper right window by making calls to the execution engine as needed to get current state. Main also makes function calls to create hardware state information snapshots and compare values to the previous snapshot so it can highlight what has changed.
5. The UI can display a dialog to gather information on what GPIO pins should be used to display timeline diagrams. When the timeline is to be displayed, it then makes a callback to Main which gets the PIN information history from the execution engine and then makes a call to the UI to display the timeline. This bit of back-and-forth between Main and UI helps ensure the independence of UI from the execution engine. 

## Simpio Parser

### Overview

The Parser is composed of a a lexical scanner and a language grammar implemented in Flex and Bison (which are modern versions of the old Lex and Yacc tools to implement language processing).

## 

### Files

- simpio.l is the lexical scanner program that is compiled using Flex. All the individual keywords as well as rules for valid symbols and numbers are in this file.
- simpio.y is the actual parser. It has some functions that can be called to do the parsing but most of this file is a series of rules for the complete PIO instruction set grammar. Embedded in the rules are function calls to add instructions and configure hardware values as needed. 

### Notes

The Parser has one function that is intended to be called when the user interface is running, and there is another function that writes detailed output to the terminal window and a summary of all configuration and instructions parsed. This second function is intended for debugging syntax issues when the user interface does not provide sufficient information.

There is a global instruction structure  that is filled out as the rules progress, and at the end of parsing each valid instruction, this global stuct is passed to the execution engine to be added to the current PIO's instruction list, and then the global struct is reinitialized before the next instruction is parsed.

## UI

### Overview

The UI creates the three windows, but only manages the content of the edit window (the left hand window).  The edit window is actually managed by a sub-component of the UI called editor. When the UI is initialized, it expects a structure to be filled out with a bunch of callback functions that it uses when the user presses a function key, except for the timeline dialog which it displays and processes directly.

### Files

1. uh.c contains the main UI functions to create the windows and process keyboard input. It call the editor functions for everything except the function keys which trigger callbacks to be executed. It also provides the functions to display a timeline
2. editor.c contains all the functions that ui calls for moving the cursor, scrolling, inserting and deleting characters. This could possibly add basic edit functionality to other Ncurses applications. It is a very basic editor but it is very tiny, only a a couple of hundred lines of code. 

### Notes

editor first processes the input file to identify where each line ends, so that the up and down cursor keys can go up and down one line, to display the line numbers, and to jump directly to breakpoints and syntax errors.

editor maintains a model of the file being displayed/edited and also a view of it on the screen in a structure that the calling application owns. This would allow multiple edit windows to be supported which could allow multiple PIO program files to be handled possibly in the future.

## Execution Engine

### Overview

This is the core part of Simpio which maintains all hardware state information and processes instructions, both user and PIO instructions.

### Files

1. hardware.c is a container component which holds all the hardware state information for all the PIOs, state machines, user processors, GPIOs, etc. It does not do any processing but rather just provides functions to set and get hardware state information.
2. instruction.c is another container component that holds state information for each instruction that is being processed. It does not provide any processing of instructions but rather just provides functions to get, set, and print instructions. 
3. execution.c does the actually instruction processing. It uses the instruction information from instruction.c to update the hardware state information in hardware.c as it processes each instruction. It also contains a simple round-robin scheduler to run instructions for each state machine and user processor one at a time; this keeps Simpio a simple single threaded application. 
4. hardware_change.c provides containers to store GPIO history and a snapshot of all previous hardware state, and also provides functions to add to GPIO history and create a new snapshot. It also provides functions to find out what has changed since the previous snapshot and to retrieve GPIO history. 

### Notes

instruction.c does not use the binary format that real PIO programming uses, but it does include an instruction "decoder" that will take the binary format that real PIO uses and create a corresponding instruction in Simpio format. This is for EXEC destinations of PIO instructions.

instruction.c maintains a lot of state information about the current processing of an instruction, including which state machine is executing the instruction. An alternative approach would be to create a state_machine.c component but maintaining everything in the instruction_t structure makes execution more straightforward because everything needed to perform one cycle of execution of an instruction is contained in the instruction itself. In fact, there is a lot of redundant information in the instruction structure  such as both the string label and line number of jump locations (one could be derived from the other), but this makes execution more straightforward. Basically, any information that would help make  execution processing easier is included (emphasizing simplicity over efficiency for the simulation logic).



## Main

Main ties everything together. It takes command line arguments, starts the UI, and contains the callback functions that UI uses, as well as a routine to display all the hardware state information in the upper right window of Simpio.

calls parser, UI and engine functions as needed . The only file is main.c which as the callback functions that I calls as well 

## Building

Simpio relies on Flex, Bison, Ncurses, make, and a C compiler. These can all be obtained on a Ubuntu system with the following:

```
TBD, include apt install instruction to get everything needed
```

Once all the prerequisites are installed, there is just one Makefile to build everything. It is a very simple Makefile that is hard coded rather than built from something like Cmake or auto-tools. Simpio is pretty small so any sophisticated build system seemed like a bit of an overkill, but is a possible future enhancement.

The default is to static link everything. The only dynamic dependency, besides a standard C library is an Ncurses library (neither Flex nor Bison require a run-time library), and both of these seem to work well statically linked. Even with everything statically linked plus all the UI  strings and debug information included, the executable is only about 1.5MB. Since the whole point of Simpio is to provide something that makes it is as simple and easy as possible to get started learning (or just playing around with) PIO programming, having a single executable that could be run from any Linux command line without having to  build or install anything is attractive. 

But creating a dynamic linked version, with or without debug information, can be done by just commenting out some lines in the Makefile and uncommenting a few other lines.