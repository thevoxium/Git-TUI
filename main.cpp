
#include <string.h>
#include <ncurses.h>
#include <git2.h>
#include <iostream>


using namespace std;

#define MAX_ROWS 1000

int main(){
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

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
  git_revwalk_sorting(walker, GIT_SORT_TIME | GIT_SORT_REVERSE);

  git_oid oid;

  int commit_message_count = 0;

  while(git_revwalk_next(&oid, walker) == 0){
    
    if (git_commit_lookup(&commit, repo, &oid) == 0){
      const char* message = git_commit_message(commit);
      commit_message[commit_message_count] = strdup(message);
    }
    commit_message_count++;

    git_commit_free(commit);
  }
 
  int y_coordinate_commit = 1;
  
  int starting_line = 0;
  int lines_to_display = commit_window_size-2;


  while(1){

    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 1, "Commit History");

    for (int i=starting_line; i<starting_line+lines_to_display && i<commit_message_count; i++){
      mvwprintw(win, i-starting_line+1, 1, "%s", commit_message[i]);
      y_coordinate_commit++;
    }
    wrefresh(win);

    int ch = getch();
    if (ch == 'q'){
      break;
    }else if(ch ==  KEY_DOWN){
      if (starting_line+lines_to_display < commit_message_count) starting_line++;
    }else if (ch == KEY_UP){
      if (starting_line > 0) starting_line--;
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

