//
// kernel.cpp
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include "lisp.h"

#include <iostream>
using namespace std;

#include <unistd.h>

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	CStdlibAppStdio ("circleLisp")
{	
    s_pThis = this;
}


CKernel *CKernel::Get (void)
{
  assert (s_pThis != 0);
  return s_pThis;
}

CStdlibApp::TShutdownMode CKernel::Run (void)
{
	int NELEM = 8191;
    ht_init(NELEM);
    init_env();
    struct object *exp;
    int i;
    
    printf(
        "Microlisp intrepreter - (c) Michael Lazear 2016-2019, MIT License\n");
    
    load_file(cons(make_symbol("lib.scm"), NIL));

    printf("\n");

    printf("HELP:Use (ls),(cd \"folderName\"),(mkdir \"folderName\"),(unlink \"fileOrFolderName\") to manage FileSystem\n");
    printf("(ls) can only display files in current folder.\"folderName\" should write in double quotation marks\n");

    printf("\n");

    //printf("%d\n",mConsole.GetOptions());

    string prompt;
    char buf[1024];

    for (;;) {
        getcwd(buf, sizeof(buf));

        prompt.clear();

        prompt.append("[");
        prompt.append(buf);
        prompt.append("]> ");
        //printf("user> ");
        cout<<prompt;
        exp = eval(read_exp(stdin), ENV);
        if (!null(exp)) {
            print_exp("====>", exp);
            printf("\n");
        }
    }

	return ShutdownHalt;
}
