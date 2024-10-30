
#include <string.h>
#include <ncurses.h>
#include <git2.h>
#include <iostream>
#include <ctime> 
#include <string> 
#include <vector> 

using namespace std;

#define MAX_ROWS 1000


struct CommitInfo{
  string message;
  string author;
  string email;
  string commit_id;
  time_t commit_time;
};

int main(){
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  
  vector<CommitInfo> commitList;

  char* commit_message[MAX_ROWS];

  git_libgit2_init();
  git_repository* repo = NULL;
  git_revwalk* walker = NULL;
  git_commit* commit = NULL;

  int maxX, maxY;
  int commit_window_size;
  getmaxyx(stdscr, maxY, maxX);
  commit_window_size = maxY/2;

  WINDOW* win = newwin(commit_window_size, maxX-2, 1, 1);
  refresh();
  box(win, 0, 0);

  mvwprintw(win, 0, 1, "Commit History");
  wrefresh(win);


  WINDOW* commit_info_window = newwin(commit_window_size-4, maxX-2, commit_window_size + 4, 1);
  box(commit_info_window, 0, 0);
  mvwprintw(commit_info_window, 0, 1, "Commit Info");
  wrefresh(commit_info_window);

  int error = git_repository_open(&repo, "/Users/anshul/interviewer/");

  if(error < 0){
    const git_error* e = git_error_last();
    printw("Error %d/%d: %s\n", error, e->klass, e->message);
  }else{
    printw("Repo opened");
  }

  
  if (git_revwalk_new(&walker, repo) < 0){
    const git_error* e = git_error_last();
    printw("Error %s", e->message);
  }else{
    printw("revwalker initialised");
  }

  git_revwalk_push_head(walker);
  git_revwalk_sorting(walker, GIT_SORT_TIME);

  git_oid oid;

  int commit_message_count = 0;

  while(git_revwalk_next(&oid, walker) == 0){
    
    if (git_commit_lookup(&commit, repo, &oid) == 0){
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
 
  int y_coordinate_commit = 1;
  
  int starting_line = 0;
  int lines_to_display = commit_window_size-2;
  
  int cursor_position = 1;
  int commit_info_window_count = 0;
  while(1){
    cursor_position = max(min(cursor_position, commit_window_size-2), 1);
    starting_line = max(min(starting_line, commit_message_count - lines_to_display), 0);
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 1, "Commit History");

    for (int i=starting_line; i<starting_line+lines_to_display && i<commit_message_count; i++){
      mvwprintw(win, i-starting_line+1, 1, "%s", commitList[i].message.c_str());
      y_coordinate_commit++;
    }

    mvwchgat(win, cursor_position, 1, -1, A_REVERSE, 0, NULL);
    mvwprintw(commit_info_window, 1, 1, "%s", commitList[commit_info_window_count].message.c_str());
    wrefresh(commit_info_window);
    wrefresh(win);
    

    int ch = getch();
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
  
  for (int i=0; i<commit_message_count; i++){
    free(commit_message[i]);
  }

  int exit_cmd = getch();

  if (exit_cmd == 'x'){
    endwin();
    git_revwalk_free(walker);
    git_repository_free(repo);
    git_libgit2_shutdown();
  }

  return 0;
} 

