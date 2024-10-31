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

int main() {
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

    // Enable scrolling for windows
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


    // Open repository and initialize walker
    int error = git_repository_open(&repo, "/Users/anshul/interviewer/");
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


    int window_flag = 0;
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
                            vector<string> file_changed_list;
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
                            
                            // Display files
                            int current_y_files_changed = 1;
                            for (const auto& s : file_changed_list) {
                                mvwprintw(files_changed, current_y_files_changed++, 2, "%s", s.c_str());
                            }

                        }
                    }
                }
            } else {
                // For initial commit, show all files as added
                error = git_diff_tree_to_tree(&diff, repo, NULL, current_tree, NULL);
                if (error == 0) {
                    vector<string> file_changed_list;
                    size_t num_deltas = git_diff_num_deltas(diff);
                    for (size_t i = 0; i < num_deltas; ++i) {
                        const git_diff_delta* delta = git_diff_get_delta(diff, i);
                        file_changed_list.push_back("[A] " + string(delta->new_file.path));
                    }
                    
                    int current_y_files_changed = 1;
                    for (const auto& s : file_changed_list) {
                        mvwprintw(files_changed, current_y_files_changed++, 2, "%s", s.c_str());
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
            mvwchgat(files_changed, 1, 1, maxX/2 - 3, A_REVERSE, 2, NULL);
        
        }

        box(files_changed, 0, 0);
        wrefresh(files_changed);

        
        // Handle keyboard input
        int ch = getch();
        
        if(ch == '\t'){
          window_flag++;
          if(window_flag > 1) window_flag = 0;
        }

        if (ch == 'q') {
            break;
        } else if (ch == KEY_DOWN || ch == 'k') {
            if (commit_info_window_count < commit_message_count - 1) {
                commit_info_window_count++;
                if (cursor_position < lines_to_display) {
                    cursor_position++;
                } else if (starting_line + lines_to_display < commit_message_count) {
                    starting_line++;
                }
            }
        } else if (ch == KEY_UP || ch == 'j') {
            if (commit_info_window_count > 0) {
                commit_info_window_count--;
                if (cursor_position > 1) {
                    cursor_position--;
                } else if (starting_line > 0) {
                    starting_line--;
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
