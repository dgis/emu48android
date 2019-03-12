/*
 *   mru.c
 *
 *   This file is part of Emu48
 *
 *   Copyright (C) 2007 Christoph Gieﬂelink
 *
 */
#include "pch.h"
#include "resource.h"
#include "Emu48.h"

static TCHAR  szOriginal[MAX_PATH] = _T("");

static LPTSTR *ppszFiles = NULL;			// pointer to MRU table
static UINT   nEntry = 0;					// no. of MRU entries

static BOOL GetMenuPosForId(HMENU hMenu, UINT nItem, HMENU *phMruMenu, UINT *pnMruPos)
{
	HMENU hSubMenu;
	UINT  i,nID,nMaxID;

	nMaxID = GetMenuItemCount(hMenu);
	for (i = 0; i < nMaxID; ++i)
	{
		nID = GetMenuItemID(hMenu,i);		// get ID

		if (nID == 0) continue;				// separator or invalid command

		if (nID == (UINT)-1)				// possibly a popup menu
		{
			hSubMenu = GetSubMenu(hMenu,i);	// try to get handle to popup menu
			if (hSubMenu != NULL)			// it's a popup menu
			{
				// recursive search
				if (GetMenuPosForId(hSubMenu,nItem,phMruMenu,pnMruPos))
					return TRUE;
			}
			continue;
		}

		if (nID == nItem)					// found ID
		{
			*phMruMenu = hMenu;				// remember menu and position
			*pnMruPos = i;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL MruInit(UINT nNum)
{
	HMENU hMenu = GetMenu(hWnd);			// main menu
	if (hMenu == NULL) return FALSE;		// failed, no menu bar

	_ASSERT(ppszFiles == NULL);				// MRU already initialized

	// no. of files in MRU list
	nEntry = ReadSettingsInt(_T("MRU"),_T("FileCount"),nNum);

	if (nEntry > 0)							// allocate MRU table
	{
		// create MRU table
		if ((ppszFiles = (LPTSTR *) malloc(nEntry * sizeof(*ppszFiles))) == NULL)
			return FALSE;

		// fill each entry
		for (nNum = 0; nNum < nEntry; ++nNum)
			ppszFiles[nNum] = NULL;

		MruReadList();						// read actual MRU list
	}
	return TRUE;
}

VOID MruCleanup(VOID)
{
	UINT i;

	MruWriteList();							// write actual MRU list

	if (ppszFiles != NULL)					// table defined
	{
		for (i = 0; i < nEntry; ++i)		// cleanup each entry
		{
			if (ppszFiles[i] != NULL)
				free(ppszFiles[i]);			// cleanup entry
		}

		free(ppszFiles);					// free table
		ppszFiles = NULL;
	}
	return;
}

VOID MruAdd(LPCTSTR lpszEntry)
{
	TCHAR  szFilename[MAX_PATH];
	LPTSTR lpFilePart;
	UINT i;

	if (ppszFiles != NULL)					// MRU initialized
	{
		_ASSERT(nEntry > 0);				// must have entries

		// get full path name
		GetFullPathName(lpszEntry,ARRAYSIZEOF(szFilename),szFilename,&lpFilePart);

		// look if entry is already in table
		for (i = 0; i < nEntry; ++i)
		{
			// already in table -> quit
			if (   ppszFiles[i] != NULL
				&& lstrcmpi(ppszFiles[i],szFilename) == 0)
			{
				MruMoveTop(i);				// move to top and update menu
				return;
			}
		}

		i = nEntry - 1;						// last index
		if (ppszFiles[i] != NULL)
			free(ppszFiles[i]);				// free oldest entry

		for (; i > 0; --i)					// move old entries 1 line down
		{
			ppszFiles[i] = ppszFiles[i-1];
		}

		// add new entry to top
		ppszFiles[0] = DuplicateString(szFilename);
	}
	return;
}

VOID MruRemove(UINT nIndex)
{
	// MRU initialized and index inside valid range
	if (ppszFiles != NULL && nIndex < nEntry)
	{
		free(ppszFiles[nIndex]);			// free entry

		for (; nIndex < nEntry - 1; ++nIndex) // move below entries 1 line up
		{
			ppszFiles[nIndex] = ppszFiles[nIndex+1];
		}

		ppszFiles[nIndex] = NULL;			// clear last line
	}
	return;
}

VOID MruMoveTop(UINT nIndex)
{
	// MRU initialized and index inside valid range
	if (ppszFiles != NULL && nIndex < nEntry)
	{
		LPTSTR lpszEntry = ppszFiles[nIndex];// remember selected entry

		for (; nIndex > 0; --nIndex)		// move above entries 1 line down
		{
			ppszFiles[nIndex] = ppszFiles[nIndex-1];
		}

		ppszFiles[0] = lpszEntry;			// insert entry on top
	}
	return;
}

UINT MruEntries(VOID)
{
	return nEntry;
}

LPCTSTR MruFilename(UINT nIndex)
{
	LPCTSTR lpszName = _T("");

	// MRU initialized and index inside valid range
	if (ppszFiles != NULL && nIndex < nEntry)
	{
		lpszName = ppszFiles[nIndex];
	}
	return lpszName;
}

VOID MruUpdateMenu(HMENU hMenu)
{
	TCHAR szCurPath[MAX_PATH];
	BOOL  bEmpty;
	UINT  i;

	if (hMenu != NULL)						// have menu
	{
		HMENU hMruMenu;						// menu handle for MRU list
		UINT  nMruPos;						// insert position for MRU list

		_ASSERT(IsMenu(hMenu));				// validate menu handle

		// search for menu position of ID_FILE_MRU_FILE1
		if (GetMenuPosForId(hMenu,ID_FILE_MRU_FILE1,&hMruMenu,&nMruPos))
		{
			if (*szOriginal == 0)			// get orginal value of first MRU entry
			{
				VERIFY(GetMenuString(hMruMenu,nMruPos,szOriginal,ARRAYSIZEOF(szOriginal),MF_BYPOSITION));
			}

			if (nEntry == 0)				// kill MRU menu entry
			{
				// delete MRU menu
				DeleteMenu(hMruMenu,nMruPos,MF_BYPOSITION);

				// delete the following separator
				_ASSERT((GetMenuState(hMruMenu,nMruPos,MF_BYPOSITION) & MF_SEPARATOR) != 0);
				DeleteMenu(hMruMenu,nMruPos,MF_BYPOSITION);
			}

			if (ppszFiles != NULL)			// MRU initialized
			{
				_ASSERT(nEntry > 0);		// must have entries

				// delete all menu entries
				for (i = 0; DeleteMenu(hMenu,ID_FILE_MRU_FILE1+i,MF_BYCOMMAND) != FALSE; ++i) { }

				// check if MRU list is empty
				for (bEmpty = TRUE, i = 0; bEmpty && i < nEntry; ++i)
				{
					bEmpty = (ppszFiles[i] == NULL);
				}

				if (bEmpty)					// MRU list is empty
				{
					// fill with orginal string
					VERIFY(InsertMenu(hMruMenu,nMruPos,MF_STRING|MF_BYPOSITION|MF_GRAYED,ID_FILE_MRU_FILE1,szOriginal));
					return;
				}

				// current directory
				GetCurrentDirectory(ARRAYSIZEOF(szCurPath),szCurPath);

				for (i = 0; i < nEntry; ++i) // add menu entries
				{
					if (ppszFiles[i] != NULL) // valid entry
					{
						TCHAR  szMenuname[2*MAX_PATH+3];
						TCHAR  szFilename[MAX_PATH];
						LPTSTR lpFilePart,lpszPtr;

						// check if file path and current path is identical
						if (GetFullPathName(ppszFiles[i],ARRAYSIZEOF(szFilename),szFilename,&lpFilePart))
						{
							TCHAR szCutname[MAX_PATH];

							*(lpFilePart-1) = 0; // devide path and name

							// name is current directory -> use only name
							if (lstrcmpi(szCurPath,szFilename) == 0)
							{
								// short name view
							}
							else
							{
								// full name view
								lpFilePart = ppszFiles[i];
							}

							// cut filename to fit into menu
							GetCutPathName(lpFilePart,szCutname,ARRAYSIZEOF(szCutname),36);
							lpFilePart = szCutname;

							// adding accelerator key
							lpszPtr = szMenuname;
							*lpszPtr++ = _T('&');
							*lpszPtr++ = _T('0') + ((i + 1) % 10);
							*lpszPtr++ = _T(' ');

							// copy file to view buffer and expand & to &&
							while (*lpFilePart != 0)
							{
								if (*lpFilePart == _T('&'))
								{
									*lpszPtr++ = *lpFilePart;
								}
								*lpszPtr++ = *lpFilePart++;
							}
							*lpszPtr = 0;

							VERIFY(InsertMenu(hMruMenu,nMruPos+i,
										MF_STRING|MF_BYPOSITION,ID_FILE_MRU_FILE1+i,
										szMenuname));
						}
					}
				}
			}
		}
	}
	return;
}

VOID MruWriteList(VOID)
{
	TCHAR szItemname[32];
	UINT  i;

	if (nEntry > 0)
	{
		// no. of files in MRU list
		WriteSettingsInt(_T("MRU"),_T("FileCount"),nEntry);

		for (i = 0; i < nEntry; ++i)		// add menu entries
		{
			_ASSERT(ppszFiles != NULL);		// MRU not initialized
			wsprintf(szItemname,_T("File%d"),i+1);
			if (ppszFiles[i] != NULL)
			{
				WriteSettingsString(_T("MRU"),szItemname,ppszFiles[i]);
			}
			else
			{
				DelSettingsKey(_T("MRU"),szItemname);
			}
		}
	}
	return;
}

VOID MruReadList(VOID)
{
	TCHAR szFilename[MAX_PATH];
	TCHAR szItemname[32];
	UINT  i;

	for (i = 0; i < nEntry; ++i)			// add menu entries
	{
		_ASSERT(ppszFiles != NULL);			// MRU not initialized

		wsprintf(szItemname,_T("File%d"),i+1);
		ReadSettingsString(_T("MRU"),szItemname,_T(""),szFilename,ARRAYSIZEOF(szFilename));

		if (ppszFiles[i] != NULL)			// already filled
		{
			free(ppszFiles[i]);				// free entry
			ppszFiles[i] = NULL;			// clear last line
		}

		if (*szFilename)					// read a valid entry
		{
			ppszFiles[i] = DuplicateString(szFilename);
		}
	}
	return;
}
