# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/justin/webRTC/udp_redirect/boringssl/boringssl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/justin/webRTC/udp_redirect/boringssl/boringssl/build

# Include any dependencies generated for this target.
include ssl/pqueue/CMakeFiles/pqueue.dir/depend.make

# Include the progress variables for this target.
include ssl/pqueue/CMakeFiles/pqueue.dir/progress.make

# Include the compile flags for this target's objects.
include ssl/pqueue/CMakeFiles/pqueue.dir/flags.make

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o: ssl/pqueue/CMakeFiles/pqueue.dir/flags.make
ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o: ../ssl/pqueue/pqueue.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o"
	cd /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/pqueue.dir/pqueue.c.o   -c /home/justin/webRTC/udp_redirect/boringssl/boringssl/ssl/pqueue/pqueue.c

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pqueue.dir/pqueue.c.i"
	cd /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /home/justin/webRTC/udp_redirect/boringssl/boringssl/ssl/pqueue/pqueue.c > CMakeFiles/pqueue.dir/pqueue.c.i

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pqueue.dir/pqueue.c.s"
	cd /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /home/justin/webRTC/udp_redirect/boringssl/boringssl/ssl/pqueue/pqueue.c -o CMakeFiles/pqueue.dir/pqueue.c.s

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.requires:
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.requires

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.provides: ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.requires
	$(MAKE) -f ssl/pqueue/CMakeFiles/pqueue.dir/build.make ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.provides.build
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.provides

ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.provides.build: ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o

pqueue: ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o
pqueue: ssl/pqueue/CMakeFiles/pqueue.dir/build.make
.PHONY : pqueue

# Rule to build all files generated by this target.
ssl/pqueue/CMakeFiles/pqueue.dir/build: pqueue
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/build

ssl/pqueue/CMakeFiles/pqueue.dir/requires: ssl/pqueue/CMakeFiles/pqueue.dir/pqueue.c.o.requires
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/requires

ssl/pqueue/CMakeFiles/pqueue.dir/clean:
	cd /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue && $(CMAKE_COMMAND) -P CMakeFiles/pqueue.dir/cmake_clean.cmake
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/clean

ssl/pqueue/CMakeFiles/pqueue.dir/depend:
	cd /home/justin/webRTC/udp_redirect/boringssl/boringssl/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/justin/webRTC/udp_redirect/boringssl/boringssl /home/justin/webRTC/udp_redirect/boringssl/boringssl/ssl/pqueue /home/justin/webRTC/udp_redirect/boringssl/boringssl/build /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue /home/justin/webRTC/udp_redirect/boringssl/boringssl/build/ssl/pqueue/CMakeFiles/pqueue.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ssl/pqueue/CMakeFiles/pqueue.dir/depend
