# Git TUI - Terminal User Interface for Git Repository Exploration

A terminal-based user interface for exploring Git repositories, allowing you to browse commit history, view file changes, and inspect diffs in an interactive manner.

## Screenshot

![Git TUI Application Interface](img/app_image.png)

*The interface shows commit history (left top), commit details (left bottom), changed files (right top), and diff view (right bottom)*

## Features

- Interactive commit history browser
- Detailed commit information display
- File change tracking for each commit
- Diff viewer with syntax highlighting
- Multi-window interface with keyboard navigation
- Support for large repositories
- Color-coded changes in diff view

## Prerequisites

To build and run this application, you need the following dependencies:

- C++ compiler (g++ or clang++)
- libgit2 (for Git operations)
- ncurses (for terminal UI)
- Git repository to explore

### Installing Dependencies

#### macOS (using Homebrew):
```bash
brew install libgit2 ncurses
```

#### Ubuntu/Debian:
```bash
sudo apt-get install libgit2-dev libncurses5-dev
```

#### Fedora/RHEL:
```bash
sudo dnf install libgit2-devel ncurses-devel
```

## Building the Application

To compile the application, use the following command:

```bash
g++ -o tui main.cpp -I/opt/homebrew/opt/libgit2/include -L/opt/homebrew/opt/libgit2/lib -lncurses -lgit2
```

Note: The include and library paths might need to be adjusted based on your system's configuration.

## Usage

Run the compiled application by providing the path to a Git repository:

```bash
./tui /path/to/git/repository
```

### Navigation Controls

The interface is divided into three main panels that can be navigated using the following controls:

- `Tab`: Switch between panels (Commit History → Files Changed → Diff View)
- `↑/j`: Move cursor up/previous item
- `↓/k`: Move cursor down/next item
- `q`: Quit the application

### Panels Overview

1. **Commit History** (Left Upper Panel)
   - Displays commit messages in chronological order
   - Highlights the currently selected commit

2. **Commit Details** (Left Lower Panel)
   - Shows detailed information about the selected commit:
     - Commit ID
     - Author name and email
     - Commit date and time
     - Full commit message

3. **Files Changed** (Right Upper Panel)
   - Lists all files modified in the selected commit
   - Indicates the type of change:
     - [A] Added files
     - [M] Modified files
     - [D] Deleted files

4. **Diff View** (Right Lower Panel)
   - Shows the detailed changes for the selected file
   - Color-coded diff:
     - Green: Added lines
     - Cyan: Removed lines
     - Yellow: File and chunk headers

## Features in Detail

### Commit Browser
- Displays a scrollable list of commits
- Shows truncated commit messages for better readability
- Highlights the currently selected commit

### Commit Information
- Complete commit metadata
- Full commit message display
- Author information with email
- Formatted timestamp

### File Changes
- List of all files affected in the commit
- Change type indicators
- Scrollable file list for commits with many changes

### Diff Viewer
- Side-by-side diff display
- Syntax highlighting for changes
- Context lines around changes
- Scrollable diff view for large changes

## Error Handling

The application includes error handling for common scenarios:
- Invalid repository paths
- Repository access issues
- Memory allocation failures
- Navigation bounds checking

## Limitations

- The interface is designed for terminal windows with a minimum size
- Very long commit messages or file paths may be truncated
- Large diffs might require scrolling to view completely

## Contributing

Feel free to contribute to this project by:
1. Forking the repository
2. Creating a feature branch
3. Submitting a pull request

## License

This project is open source and available under the MIT License.

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   - Verify that all dependencies are installed
   - Check include and library paths in compilation command
   - Ensure libgit2 version compatibility

2. **Runtime Errors**
   - Confirm the repository path exists and is valid
   - Check repository permissions
   - Verify terminal window size is sufficient

3. **Display Issues**
   - Ensure terminal supports color output
   - Check terminal size requirements
   - Verify ncurses compatibility

For additional issues, please check the project's issue tracker or submit a new issue.
