#include <stdio.h>
#include <ctype.h>

#include "sys5.h"
#include "rolo.h"
#include "io.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif
#include "menu.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"


int rolo_menu_yes_no (prompt,rtn_default,help_allowed,helpfile,subject)

  char *prompt;
  int rtn_default;
  int help_allowed;
  char *helpfile, *subject;

{
  int rval;
  reask :
  rval = menu_yes_no_abort_or_help (
              prompt,ABORTSTRING,help_allowed,rtn_default
           );
  switch (rval) {
    case MENU_EOF :
      user_eof();
      break;
    case MENU_HELP :
      cathelpfile(libdir(helpfile),subject,1);
      goto reask;
      break;
    default :
      return(rval);
      break;
  }
  return(rval);
}


int rolo_menu_data_help_or_abort (prompt,helpfile,subject,ptr_response)

  char *prompt, *helpfile, *subject;
  char **ptr_response;

{
  int rval;
  reask :
  rval = menu_data_help_or_abort(prompt,ABORTSTRING,ptr_response);
  if (rval == MENU_HELP) {
     cathelpfile(libdir(helpfile),subject,1);
     goto reask;
  }
  // for the search_eof_test, rval will return MENU_EOF
  return(rval);
}


int rolo_menu_number_help_or_abort (prompt,low,high,ptr_ival)

  char *prompt;
  int low,high,*ptr_ival;

{
  int rval;
  if (MENU_EOF == (rval = menu_number_help_or_abort (
                               prompt,ABORTSTRING,low,high,ptr_ival
                           )))
     user_eof();
  return(rval);
}
