
#include <ncurses.h>
#include <git2.h>

int main(){
  initscr();
  git_libgit2_init();
  git_repository* repo = NULL;
  git_revwalk* walker = NULL;
  git_commit* commit = NULL;


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

  while(git_revwalk_next(&oid, walker) == 0){
    if (git_commit_lookup(&commit, repo, &oid) == 0){
      const char* message = git_commit_message(commit);
      printw("message: %s", message);
    }

    git_commit_free(commit);
  }

  getch();
  endwin();
  
  git_revwalk_free(walker);
  git_repository_free(repo);
  git_libgit2_shutdown();
  return 0;
} 

