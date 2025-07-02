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
#ifndef _kernel_h
#define _kernel_h

#include <circle_stdlib_app.h>

class CKernel : public CStdlibAppStdio
{
public:
	CKernel (void);

	static CKernel *Get (void);

	//CConsole have option CONSOLE_OPTION_ICANON and CONSOLE_OPTION_ECHO
	
	void setRaw(){
		mConsole.SetOptions(CONSOLE_OPTION_ECHO);
		//mConsole.SetOptions (mConsole.GetOptions () & ~CONSOLE_OPTION_ICANON);
	}

	int restoreMode()
	{
		mConsole.SetOptions(3);
	}

	void clear(){
		char *a="\x1b[H\x1b[J";
		mScreen.Write(a,strlen(a));
	}

	TShutdownMode Run (void);

private:
  static CKernel *s_pThis;
};

#endif

