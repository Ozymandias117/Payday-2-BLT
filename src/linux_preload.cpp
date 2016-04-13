#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>

#include "util/util.h"
#include "subhook/subhook.h"
#include "InitState.h"

extern "C"
{
  // __libc_start_main
  typedef int (*libc_start_sig) (int (*main) (int, char **, char **),
    int argc, char ** ubp_av, void (*init) (void), void (*fini) (void),
    void (*rtld_fini) (void), void (*stack_end));

  // main
  typedef int (*main_sig)(int argc, char** argv, char** envp);

  // Handles
  main_sig real_main = NULL;
  static const char* executable_name = "payday2_release";

  int fake_main(int argc, char** argv, char** envp)
  {
    Logging::Log("Arguments: \"" + std::string(argv[0]) + "\" PID: \"" + std::to_string(getpid()) + "\"");
    int ret = EXIT_FAILURE;
    unsigned int i = 0;
    unsigned int argv_str_len = strlen( argv[0] );
    bool paydayExe = false;

    // Ensure we're running the Payday executable
    if( argv_str_len >= strlen( executable_name ) )
    {
      while( i < argv_str_len )
      {
        if( argv[0][i] == executable_name[0] )
        {
          // found the beginning
          paydayExe = 0 == strcmp( executable_name, &argv[0][i] );
          paydayExe = true;
          break;
        }
        else
        {
          i++;
        }
      }
    }

    if( paydayExe )
      InitiateStates();

    // Call the original main function
    if( NULL != real_main )
      ret = real_main( argc, argv, envp );

    return ret;
  }

  int __libc_start_main(int (*main) (int, char **, char **), int argc, char **ubp_av,
    void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (*stack_end))
  {
    // Get the original libc_start_main
    void* real_sym = dlsym(RTLD_NEXT, "__libc_start_main");
    if( !real_sym )
    {
      Logging::Log("dlsym failed in __libc_start_main hook", Logging::LOGGING_ERROR);
      _exit( 1 );
    }

    // Save main so fake_main can call it
    real_main = main;

    return ((libc_start_sig)real_sym) (fake_main, argc, ubp_av, init, fini,
      rtld_fini, stack_end);
  }

}
