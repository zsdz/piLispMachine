//
// kernel.h
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
#include <stdio.h>

#ifndef _kernel_h
#define _kernel_h

#include <circle_stdlib_app.h>
#include <stdlib.h>

class CKernel : public CStdlibAppStdio
{
public:
	CKernel (void);

	static CKernel *Get (void);

	//CConsole have option CONSOLE_OPTION_ICANON and CONSOLE_OPTION_ECHO
	
	void setRaw(){
		//mConsole and mScreen is a nonstatic protected member in class CStdlibAppStdio,so I can't set this method static.I have to set a static member s_pThis to use these method as lupos does
		mConsole.SetOptions(CONSOLE_OPTION_ECHO);
	}

	void restoreMode()
	{
		mConsole.SetOptions(CONSOLE_OPTION_ICANON|CONSOLE_OPTION_ECHO);
	}

	void clear(){
		char *a="\x1b[H\x1b[J";
		mScreen.Write(a,strlen(a));
	}

	void printPosition(int a){
		char *b;
		sprintf(b, "%d", a);
		mScreen.Write(b,strlen(b));
	}

	TShutdownMode Run (void);

private:
  static CKernel *s_pThis;
};

#endif

