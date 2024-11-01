#!/bin/bash
# build_git_tui.sh

# Compile the program
g++ -o git-tui main.cpp \
    -I/opt/homebrew/opt/libgit2/include \
    -L/opt/homebrew/opt/libgit2/lib \
    -lncurses -lgit2

# Copy to a bin directory
mkdir -p ~/bin
cp git-tui ~/bin/

# Make it executable
chmod +x ~/bin/git-tui
