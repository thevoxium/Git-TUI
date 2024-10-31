#include <string.h>
#include <ncurses.h>
#include <git2.h>
#include <iostream>
#include <ctime> 
#include <string> 
#include <vector>
#include <iomanip>
#include <sstream>
using namespace std;

#define MAX_ROWS 1000
#define PADDING 2

struct CommitInfo {
    string message;
    string author;
    string email;
    string commit_id;
    time_t commit_time;
};

struct DiffContent {
    vector<string> lines;
    int starting_line;
    int cursor_position;
    int lines_to_display;
};


void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s <repository_path>\n", program_name);
    fprintf(stderr, "Example: %s /path/to/git/repo\n", program_name);
}

// Callback function to collect diff content
int diff_line_callback(const git_diff_delta *delta, const git_diff_hunk *hunk, 
                      const git_diff_line *line, void *payload) {
    vector<string>* diff_lines = static_cast<vector<string>*>(payload);
    
    char prefix;
    switch (line->origin) {
        case GIT_DIFF_LINE_ADDITION: prefix = '+'; break;
        case GIT_DIFF_LINE_DELETION: prefix = '-'; break;
        case GIT_DIFF_LINE_CONTEXT: prefix = ' '; break;
        case GIT_DIFF_LINE_FILE_HDR: prefix = 'F'; break;
        case GIT_DIFF_LINE_HUNK_HDR: prefix = 'H'; break;
        default: prefix = ' '; break;
    }
    
    string diff_line;
    if (prefix == 'F' || prefix == 'H') {
        diff_line = string(line->content, line->content_len);
    } else {
        diff_line = prefix + string(line->content, line->content_len);
    }
    
    // Remove trailing newlines
    if (!diff_line.empty() && diff_line.back() == '\n') {
        diff_line.pop_back();
    }
    
    diff_lines->push_back(diff_line);
    return 0;
}
// Helper function to format timestamp
string format_time(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    stringstream ss;
    ss << put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Helper function to truncate string with ellipsis
string truncate(const string& str, size_t width) {
    if (str.length() > width)
        return str.substr(0, width - 3) + "...";
    return str;
}

// Helper function to create a horizontal line
void draw_horizontal_line(WINDOW* win, int y, int x, int width, char ch = '-') {
    mvwhline(win, y, x, ch, width);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Get repository path from command line
    const char* repo_path = argv[1];
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Initialize color pairs
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // For headers
    init_pair(2, COLOR_CYAN, COLOR_BLACK);   // For highlights
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // For commit IDs
    init_pair(4, COLOR_WHITE, COLOR_BLACK);  // For normal text

    vector<CommitInfo> commitList;
    git_libgit2_init();
    git_repository* repo = NULL;
    git_revwalk* walker = NULL;
    git_commit* commit = NULL;

    // Get terminal dimensions
    int maxX, maxY;
    getmaxyx(stdscr, maxY, maxX);

    // Create windows with better proportions
    int commit_window_size = (maxY * 2) / 3;  // Top window takes 2/3 of screen
    int info_window_size = maxY - commit_window_size - 3;  // Bottom window takes remaining space

    // Create main windows
    WINDOW* win = newwin(commit_window_size, maxX/2 - 2, 1, 1);
    WINDOW* commit_info_window = newwin(info_window_size, maxX/2 - 2, commit_window_size + 2, 1);
    WINDOW* status_bar = newwin(1, maxX, maxY - 1, 0);
    WINDOW* files_changed = newwin(maxY/3, maxX/2-2, 1, maxX/2);
    WINDOW* diff_window = newwin(2*maxY/3-3, maxX/2-2, maxY/3+2, maxX/2);
    // Enable scrolling for windows
    scrollok(diff_window, TRUE);
    scrollok(win, TRUE);
    scrollok(commit_info_window, TRUE);
    scrollok(files_changed, TRUE);
    refresh();

    // Draw windows with borders and titles
    box(win, 0, 0);
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, 0, 2, "[ Commit History ]");
    wattroff(win, COLOR_PAIR(1));
    wrefresh(win);

    box(commit_info_window, 0, 0);
    wattron(commit_info_window, COLOR_PAIR(1));
    mvwprintw(commit_info_window, 0, 2, "[ Commit Details ]");
    wattroff(commit_info_window, COLOR_PAIR(1));
    wrefresh(commit_info_window);

    box(files_changed, 0, 0);
    wattron(files_changed, COLOR_PAIR(1));
    mvwprintw(files_changed, 0, 2, "[ Files Changed ]");
    wattroff(files_changed, COLOR_PAIR(1));
    wrefresh(files_changed);

    box(diff_window, 0, 0);
    wattron(diff_window, COLOR_PAIR(1));
    mvwprintw(diff_window, 0, 2, "[ Git Diff ]");
    wattroff(diff_window, COLOR_PAIR(1));
    wrefresh(diff_window);


    // Open repository and initialize walker
    int error = git_repository_open(&repo, repo_path);
    if (error < 0) {
        const git_error* e = git_error_last();
        mvwprintw(status_bar, 0, 0, "Error: %s", e->message);
        wrefresh(status_bar);
    }

    if (git_revwalk_new(&walker, repo) < 0) {
        const git_error* e = git_error_last();
        mvwprintw(status_bar, 0, 0, "Error: %s", e->message);
        wrefresh(status_bar);
    }

    git_revwalk_push_head(walker);
    git_revwalk_sorting(walker, GIT_SORT_TIME);
    git_oid oid;
    int commit_message_count = 0;

    // Collect commit information
    while (git_revwalk_next(&oid, walker) == 0) {
        if (git_commit_lookup(&commit, repo, &oid) == 0) {
            CommitInfo info;
            info.message = git_commit_message(commit);
            info.author = git_commit_author(commit)->name;
            info.email = git_commit_author(commit)->email;
            info.commit_time = git_commit_time(commit);
            info.commit_id = git_oid_tostr_s(&oid);
            commitList.push_back(info);
        }
        commit_message_count++;
        git_commit_free(commit);
    }

    int starting_line = 0;
    int lines_to_display = commit_window_size - 2;
    int cursor_position = 1;
    int commit_info_window_count = 0;

    // Status bar initial update
    wattron(status_bar, COLOR_PAIR(2));
    mvwprintw(status_bar, 0, 0, " [^/j]Up [v/k]Down [q]Quit | Total Commits: %d", commit_message_count);
    wattroff(status_bar, COLOR_PAIR(2));
    wrefresh(status_bar);
    

    int files_starting_line = 0;
    int files_cursor_position = 1;
    int files_changed_lines_to_display = maxY/3-2;
    int window_flag = 0;

    DiffContent diff_content;
    diff_content.starting_line = 0;
    diff_content.cursor_position = 1;
    diff_content.lines_to_display = 2 * maxY/3 - 5;
    string current_diff_file = "";  // Track currently displayed diff file

    vector<string> file_changed_list;
    // Main program loop
    while (1) {
        cursor_position = max(min(cursor_position, commit_window_size - 2), 1);
        starting_line = max(min(starting_line, commit_message_count - lines_to_display), 0);

        // Refresh commit history window
        werase(win);
        box(win, 0, 0);
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, 0, 2, "[ Commit History ]");
        wattroff(win, COLOR_PAIR(1));

        // Display commit messages
        for (int i = starting_line; i < starting_line + lines_to_display && i < commit_message_count; i++) {
            string truncated_message = truncate(commitList[i].message, maxX - 6);
            mvwprintw(win, i - starting_line + 1, 2, "%s", truncated_message.c_str());
        }

        // Highlight selected commit
        if (window_flag == 0){
          mvwchgat(win, cursor_position, 1, maxX - 4, A_REVERSE, 2, NULL);
        }

        box(win, 0, 0);
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, 0, 2, "[ Commit History ]");
        wattroff(win, COLOR_PAIR(1));
        wrefresh(win);

        // Refresh commit info window
        werase(commit_info_window);
        box(commit_info_window, 0, 0);
        wattron(commit_info_window, COLOR_PAIR(1));
        mvwprintw(commit_info_window, 0, 2, "[ Commit Details ]");
        wattroff(commit_info_window, COLOR_PAIR(1));

        // Display detailed commit information
        const CommitInfo& selected_commit = commitList[commit_info_window_count];
        
        wattron(commit_info_window, COLOR_PAIR(3));
        mvwprintw(commit_info_window, 2, 2, "Commit ID: ");
        wattroff(commit_info_window, COLOR_PAIR(3));
        mvwprintw(commit_info_window, 2, 13, "%s", selected_commit.commit_id.c_str());

        wattron(commit_info_window, COLOR_PAIR(2));
        mvwprintw(commit_info_window, 3, 2, "Author:   ");
        wattroff(commit_info_window, COLOR_PAIR(2));
        mvwprintw(commit_info_window, 3, 13, "%s <%s>", 
                 selected_commit.author.c_str(), 
                 selected_commit.email.c_str());

        wattron(commit_info_window, COLOR_PAIR(2));
        mvwprintw(commit_info_window, 4, 2, "Date:     ");
        wattroff(commit_info_window, COLOR_PAIR(2));
        mvwprintw(commit_info_window, 4, 13, "%s", 
                 format_time(selected_commit.commit_time).c_str());

        // Draw separator
        draw_horizontal_line(commit_info_window, 5, 1, maxX - 4);

        wattron(commit_info_window, COLOR_PAIR(2));
        mvwprintw(commit_info_window, 6, 2, "Message:");
        wattroff(commit_info_window, COLOR_PAIR(2));
        
        // Handle multi-line commit messages
        string msg = selected_commit.message;
        size_t pos = 0;
        int line = 7;
        string delimiter = "\n";
        while ((pos = msg.find(delimiter)) != string::npos) {
            string token = msg.substr(0, pos);
            mvwprintw(commit_info_window, line++, 4, "%s", token.c_str());
            msg.erase(0, pos + delimiter.length());
        }
        if (!msg.empty()) {
            mvwprintw(commit_info_window, line, 4, "%s", msg.c_str());
        }

        // Refresh windows
        wrefresh(commit_info_window);
        wrefresh(win);
        
        
                // code for files Changed
        werase(files_changed);
        box(files_changed, 0, 0);
        wattron(files_changed, COLOR_PAIR(1));
        mvwprintw(files_changed, 0, 2, "[ Files Changed ]");
        wattroff(files_changed, COLOR_PAIR(1));

        git_commit* current_commit = NULL;
        git_commit* parent_commit = NULL;
        git_tree* current_tree = NULL;
        git_tree* parent_tree = NULL;
        git_diff* diff = NULL;

        // Get current commit
        git_oid current_oid;
        git_oid_fromstr(&current_oid, commitList[commit_info_window_count].commit_id.c_str());
        error = git_commit_lookup(&current_commit, repo, &current_oid);

        if (error == 0) {
            // Get current commit tree
            error = git_commit_tree(&current_tree, current_commit);
            
            // Get parent commit (if any)
            if (git_commit_parentcount(current_commit) > 0) {
                error = git_commit_parent(&parent_commit, current_commit, 0);
                if (error == 0) {
                    error = git_commit_tree(&parent_tree, parent_commit);
                    
                    // Compare trees
                    if (error == 0) {
                        error = git_diff_tree_to_tree(&diff, repo, parent_tree, current_tree, NULL);
                        
                        if (error == 0) {
                            file_changed_list.clear();
                            size_t num_deltas = git_diff_num_deltas(diff);
                            
                            for (size_t i = 0; i < num_deltas; ++i) {
                                const git_diff_delta* delta = git_diff_get_delta(diff, i);
                                string status;
                                switch (delta->status) {
                                    case GIT_DELTA_ADDED: status = "[A] "; break;
                                    case GIT_DELTA_MODIFIED: status = "[M] "; break;
                                    case GIT_DELTA_DELETED: status = "[D] "; break;
                                    default: status = "[?] "; break;
                                }
                                file_changed_list.push_back(status + delta->new_file.path);
                            }
                           
                            files_cursor_position = max(min(files_cursor_position, files_changed_lines_to_display), 1);
                            files_starting_line = max(min(files_starting_line, (int)file_changed_list.size() - files_changed_lines_to_display), 0);

                            int current_y_files_changed = 1;
                            for (int i = files_starting_line; 
                                 i < files_starting_line + files_changed_lines_to_display && i < file_changed_list.size(); 
                                 i++) {
                                mvwprintw(files_changed, current_y_files_changed++, 2, "%s", file_changed_list[i].c_str());
                            }
                        }
                    }
                }
            } else {
                // For initial commit, show all files as added
                error = git_diff_tree_to_tree(&diff, repo, NULL, current_tree, NULL);
                if (error == 0) {
                    file_changed_list.clear();
                    size_t num_deltas = git_diff_num_deltas(diff);
                    for (size_t i = 0; i < num_deltas; ++i) {
                        const git_diff_delta* delta = git_diff_get_delta(diff, i);
                        file_changed_list.push_back("[A] " + string(delta->new_file.path));
                    }
                    

                    int current_y_files_changed = 1;
                    for (int i=0; i<file_changed_list.size() && i<=files_changed_lines_to_display; i++) {
                        mvwprintw(files_changed, current_y_files_changed++, 2, "%s", file_changed_list[i].c_str());
                    }
                }
            }
        }

        // Cleanup
        if (diff) git_diff_free(diff);
        if (current_tree) git_tree_free(current_tree);
        if (parent_tree) git_tree_free(parent_tree);
        if (current_commit) git_commit_free(current_commit);
        if (parent_commit) git_commit_free(parent_commit);
         
        if (window_flag == 1) {
            mvwchgat(files_changed, files_cursor_position, 1, maxX/2 - 3, A_REVERSE, 2, NULL);
        }       
     
        box(files_changed, 0, 0);
        wattron(files_changed, COLOR_PAIR(1));
        mvwprintw(files_changed, 0, 2, "[ Files Changed ]");
        wattroff(files_changed, COLOR_PAIR(1));
        wrefresh(files_changed);



        if (window_flag == 1 && !file_changed_list.empty()) {
            string selected_file = file_changed_list[files_starting_line + files_cursor_position - 1];
            string file_path = selected_file.substr(4);  // Remove the status prefix [A]/[M]/[D]
            
            // Only update diff if we've selected a different file
            if (file_path != current_diff_file) {
                current_diff_file = file_path;
                diff_content.lines.clear();
                diff_content.starting_line = 0;
                diff_content.cursor_position = 1;
                
                git_commit* current_commit = NULL;
                git_commit* parent_commit = NULL;
                git_tree* current_tree = NULL;
                git_tree* parent_tree = NULL;
                git_diff* diff = NULL;
                
                // Get current commit
                git_oid current_oid;
                git_oid_fromstr(&current_oid, commitList[commit_info_window_count].commit_id.c_str());
                git_commit_lookup(&current_commit, repo, &current_oid);
                
                if (current_commit) {
                    git_commit_tree(&current_tree, current_commit);
                    
                    // Set up diff options to filter for the selected file
                    git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
                    git_strarray pathspec = {0};
                    const char* paths[] = { file_path.c_str() };
                    pathspec.strings = const_cast<char**>(paths);
                    pathspec.count = 1;
                    diff_opts.pathspec = pathspec;
                    diff_opts.context_lines = 3;  // Number of context lines around changes
                    
                    if (git_commit_parentcount(current_commit) > 0) {
                        git_commit_parent(&parent_commit, current_commit, 0);
                        git_commit_tree(&parent_tree, parent_commit);
                        
                        // Generate diff for specific file
                        git_diff_tree_to_tree(&diff, repo, parent_tree, current_tree, &diff_opts);
                    } else {
                        // For initial commit, compare with empty tree
                        git_diff_tree_to_tree(&diff, repo, NULL, current_tree, &diff_opts);
                    }
                    
                    if (diff) {
                        // Add file header
                        diff_content.lines.push_back("diff --git a/" + file_path + " b/" + file_path);
                        
                        // Collect diff content
                        git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diff_line_callback, &diff_content.lines);
                    }
                }
                
                // Cleanup
                if (diff) git_diff_free(diff);
                if (current_tree) git_tree_free(current_tree);
                if (parent_tree) git_tree_free(parent_tree);
                if (current_commit) git_commit_free(current_commit);
                if (parent_commit) git_commit_free(parent_commit);
            }
        }

        werase(diff_window);
        box(diff_window, 0, 0);
        wattron(diff_window, COLOR_PAIR(1));
        mvwprintw(diff_window, 0, 2, "[ Git Diff: %s ]", current_diff_file.c_str());
        wattroff(diff_window, COLOR_PAIR(1));

        // Add padding at the top
        int content_start_y = 2;  // Start content with 1 line padding after title
        int left_padding = 3;     // Increase left padding from 2 to 3
        
        // Calculate available lines considering padding
        diff_content.lines_to_display = (2 * maxY/3 - 6);  // Reduce by 1 to prevent overlap with bottom border

        // Display diff content with scrolling and color coding
        for (int i = diff_content.starting_line; 
             i < diff_content.starting_line + diff_content.lines_to_display && i < diff_content.lines.size(); 
             i++) {
            if (!diff_content.lines[i].empty()) {
                char first_char = diff_content.lines[i][0];
                switch (first_char) {
                    case '+':
                        wattron(diff_window, COLOR_PAIR(1));  // Green for additions
                        break;
                    case '-':
                        wattron(diff_window, COLOR_PAIR(2));  // Red for deletions
                        break;
                    case 'F':
                    case 'H':
                        wattron(diff_window, COLOR_PAIR(3));  // Yellow for headers
                        break;
                }
            }
            
            // Calculate y position with padding
            int current_y = content_start_y + (i - diff_content.starting_line);
            
            // Clear the entire line first to prevent character overlap
            wmove(diff_window, current_y, 1);
            wclrtoeol(diff_window);
            
            // Print the line with left padding
            mvwprintw(diff_window, current_y, left_padding, "%s", 
                     diff_content.lines[i].c_str());
            
            wattroff(diff_window, COLOR_PAIR(1));
            wattroff(diff_window, COLOR_PAIR(2));
            wattroff(diff_window, COLOR_PAIR(3));

        // Display diff content
      }


        if (window_flag == 2 && !diff_content.lines.empty()) {
            mvwchgat(diff_window, content_start_y + diff_content.cursor_position - 1, 
                    1, maxX/2 - 2, A_REVERSE, 2, NULL);
        }

        // Redraw box to ensure clean borders
        box(diff_window, 0, 0);
        wattron(diff_window, COLOR_PAIR(1));
        mvwprintw(diff_window, 0, 2, "[ Git Diff: %s ]", current_diff_file.c_str());
        wattroff(diff_window, COLOR_PAIR(1));

        wrefresh(diff_window);



       // Handle keyboard input
        int ch = getch();
        
                
        if (ch == '\t') {
            window_flag++;
            if (window_flag > 2) window_flag = 0;
            
            // Reset cursor positions when switching windows
            if (window_flag == 1) {
                files_starting_line = 0;
                files_cursor_position = 1;
            } else if (window_flag == 2) {
                diff_content.starting_line = 0;
                diff_content.cursor_position = 1;
            }
        }
        
        if (ch == 'q') {
            break;
        } else if (ch == KEY_DOWN || ch == 'k') {
            if (window_flag == 0) {
                if (commit_info_window_count < commit_message_count - 1) {
                    commit_info_window_count++;
                    if (cursor_position < lines_to_display) {
                        cursor_position++;
                    } else if (starting_line + lines_to_display < commit_message_count) {
                        starting_line++;
                    }
                }
            } else if (window_flag == 1) {
                if (files_cursor_position < files_changed_lines_to_display && 
                    files_starting_line + files_cursor_position < file_changed_list.size()) {
                    files_cursor_position++;
                } else if (files_starting_line + files_changed_lines_to_display < file_changed_list.size()) {
                    files_starting_line++;
                }
            }
            else if (window_flag == 2 && !diff_content.lines.empty()) {
                if (diff_content.cursor_position < diff_content.lines_to_display && 
                    diff_content.starting_line + diff_content.cursor_position < diff_content.lines.size()) {
                    diff_content.cursor_position++;
                } else if (diff_content.starting_line + diff_content.lines_to_display < diff_content.lines.size()) {
                    diff_content.starting_line++;
                }
            }
        } else if (ch == KEY_UP || ch == 'j') {
            if (window_flag == 0) {
                if (commit_info_window_count > 0) {
                    commit_info_window_count--;
                    if (cursor_position > 1) {
                        cursor_position--;
                    } else if (starting_line > 0) {
                        starting_line--;
                    }
                }
            } else if (window_flag == 1) {
                if (files_cursor_position > 1) {
                    files_cursor_position--;
                } else if (files_starting_line > 0) {
                    files_starting_line--;
                }
            }

            else if (window_flag == 2) {
                if (diff_content.cursor_position > 1) {
                    diff_content.cursor_position--;
                } else if (diff_content.starting_line > 0) {
                    diff_content.starting_line--;
                }
            }
        }



    }

    // Cleanup
    endwin();
    git_revwalk_free(walker);
    git_repository_free(repo);
    git_libgit2_shutdown();
    
    return 0;
}
