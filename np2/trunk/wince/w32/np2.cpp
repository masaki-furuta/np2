#include	"compiler.h"
#include	"resource.h"
#include	"strres.h"
#include	"np2.h"
#include	"dosio.h"
#include	"commng.h"
#include	"inputmng.h"
#include	"scrnmng.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"winkbd.h"
#include	"ini.h"
#include	"memory.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"pc9861k.h"
#include	"mpu98ii.h"
#include	"bios.h"
#include	"scrndraw.h"
#include	"sound.h"
#include	"beep.h"
#include	"s98.h"
#include	"diskdrv.h"
#include	"fddfile.h"
#include	"font.h"
#include	"timing.h"
#include	"statsave.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"


static const TCHAR szAppCaption[] = STRLITERAL("Neko Project II (PocketPC)");
static const TCHAR szClassName[] = STRLITERAL("NP2-MainWindow");


		NP2OSCFG	np2oscfg = {0, 2, 0, 0};
		HWND		hWndMain;
		HINSTANCE	hInst;
		HINSTANCE	hPrev;
		char		modulefile[MAX_PATH];
		BOOL		sysrunning;

static	UINT		framecnt;
static	UINT		waitcnt;
static	UINT		framemax = 1;


// ---- resume

static void getstatfilename(char *path, const char *ext, int size) {

	file_cpyname(path, modulefile, size);
	file_cutext(path);
	file_catname(path, str_dot, size);
	file_catname(path, ext, size);
}

static int flagsave(const char *ext) {

	int		ret;
	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	return(ret);
}

static void flagdelete(const char *ext) {

	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

static int flagload(const char *ext, const char *title, BOOL force) {

	int		ret;
	int		id;
	char	path[MAX_PATH];
	char	buf[1024];
	char	buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	id = DID_YES;
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~NP2FLAG_DISKCHG)) {
		menumbox("Couldn't restart", title, MBOX_OK | MBOX_ICONSTOP);
		id = DID_NO;
	}
	else if ((!force) && (ret & NP2FLAG_DISKCHG)) {
		wsprintf(buf2, "Conflict!\n\n%s\nContinue?", buf);
		id = menumbox(buf2, title, MBOX_YESNOCAN | MBOX_ICONQUESTION);
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return(id);
}


// ---- proc

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT	ps;
	HDC			hdc;

	switch (msg) {
		case WM_CREATE:
//			WINNLSEnableIME(hWnd, FALSE);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			if (!sysrunning) {
				scrnmng_clear(TRUE);
			}
			else {
				scrndraw_redraw();
			}
			EndPaint(hWnd, &ps);
			break;

		case WM_KEYDOWN:
			if (wParam == VK_F11) {
				if (menuvram == NULL) {
					sysmenu_menuopen(0, 0, 0);
				}
				else {
					menubase_close();
				}
			}
			else {
				winkbd_keydown(wParam, lParam);
			}
			break;

		case WM_KEYUP:
			winkbd_keyup(wParam, lParam);
			break;

		case WM_SYSKEYDOWN:
			winkbd_keydown(wParam, lParam);
			break;

		case WM_SYSKEYUP:
			winkbd_keyup(wParam, lParam);
			break;

		case WM_MOUSEMOVE:
			if (scrnmng_mousepos(&lParam) == SUCCESS) {
				if (menuvram == NULL) {
				}
				else {
					menubase_moving(LOWORD(lParam), HIWORD(lParam), 0);
				}
			}
			break;

		case WM_LBUTTONDOWN:
			if (scrnmng_mousepos(&lParam) == SUCCESS) {
				if (menuvram == NULL) {
				}
				else {
					menubase_moving(LOWORD(lParam), HIWORD(lParam), 1);
				}
			}
			break;

		case WM_LBUTTONUP:
			if (scrnmng_mousepos(&lParam) == SUCCESS) {
				if (menuvram == NULL) {
					sysmenu_menuopen(0, LOWORD(lParam), HIWORD(lParam));
				}
				else {
					menubase_moving(LOWORD(lParam), HIWORD(lParam), 2);
				}
			}
			break;

		case WM_ENTERSIZEMOVE:
			soundmng_stop();
			break;

		case WM_EXITSIZEMOVE:
			soundmng_play();
			break;

		case WM_CLOSE:
			taskmng_exit();
			break;

		case WM_DESTROY:
//			PostQuitMessage(0);
			break;

#if defined(WAVEMNG_CBMAIN)
		case MM_WOM_DONE:
			soundmng_cb(MM_WOM_DONE, (HWAVEOUT)wParam, (WAVEHDR *)lParam);
			break;
#endif

		default:
			return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0L);
}


#define	framereset(cnt)		framecnt = 0

static void processwait(UINT cnt) {

	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	}
	else {
		Sleep(1);
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,
										LPTSTR lpszCmdLine, int nCmdShow) {

	HWND		hWnd;
	WNDCLASS	np2;
	int			id;
	MSG			msg;

	hWnd = FindWindow(szClassName, NULL);
	if (hWnd != NULL) {
		ShowWindow(hWnd, SW_RESTORE);
		SetForegroundWindow(hWnd);
		return(FALSE);
	}

	GetModuleFileName(NULL, modulefile, sizeof(modulefile));
	dosio_init();
	file_setcd(modulefile);
	initload();

//	srand((unsigned)time(NULL));

	hInst = hInstance;
	hPrev = hPreInst;
	TRACEINIT();

	inputmng_init();
	keystat_reset();

	if (!hPreInst) {
		np2.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
		np2.lpfnWndProc = WndProc;
		np2.cbClsExtra = 0;
		np2.cbWndExtra = 0;
		np2.hInstance = hInstance;
		np2.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		np2.hCursor = LoadCursor(NULL, IDC_ARROW);
		np2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		np2.lpszMenuName = NULL;
		np2.lpszClassName = szClassName;
		if (!RegisterClass(&np2)) {
			return(FALSE);
		}
	}

	hWnd = CreateWindow(szClassName, szAppCaption,
						WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION |
						WS_MINIMIZEBOX,
						0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT,
						NULL, NULL, hInstance, NULL);
	hWndMain = hWnd;
	if (hWnd == NULL) {
		goto np2main_err1;
	}

	scrnmng_initialize();

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (sysmenu_create() != SUCCESS) {
		DestroyWindow(hWnd);
		goto np2main_err1;
	}
	if (scrnmng_create(hWnd, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT)
																!= SUCCESS) {
		MessageBox(hWnd, "Couldn't create DirectDraw Object",
									szAppCaption, MB_OK | MB_ICONSTOP);
		DestroyWindow(hWnd);
		goto np2main_err2;
	}

	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	S98_init();

	scrndraw_redraw();
	pccore_reset();

	if (np2oscfg.resume) {
		id = flagload(str_sav, str_resume, FALSE);
		if (id == DID_CANCEL) {
			DestroyWindow(hWndMain);
			goto np2main_err3;
		}
	}

	sysrunning = TRUE;

	while(taskmng_isavail()) {
		if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage(&msg, NULL, 0, 0)) {
				break;
			}
			if ((msg.message != WM_SYSKEYDOWN) &&
				(msg.message != WM_SYSKEYUP)) {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);
		}
		else {
			if (np2oscfg.NOWAIT) {
				pccore_exec(framecnt == 0);
				if (np2oscfg.DRAW_SKIP) {			// nowait frame skip
					framecnt++;
					if (framecnt >= np2oscfg.DRAW_SKIP) {
						processwait(0);
					}
				}
				else {							// nowait auto skip
					framecnt = 1;
					if (timing_getcount()) {
						processwait(0);
					}
				}
			}
			else if (np2oscfg.DRAW_SKIP) {		// frame skip
				if (framecnt < np2oscfg.DRAW_SKIP) {
					pccore_exec(framecnt == 0);
					framecnt++;
				}
				else {
					processwait(np2oscfg.DRAW_SKIP);
				}
			}
			else {								// auto skip
				if (!waitcnt) {
					UINT cnt;
					pccore_exec(framecnt == 0);
					framecnt++;
					cnt = timing_getcount();
					if (framecnt > cnt) {
						waitcnt = framecnt;
						if (framemax > 1) {
							framemax--;
						}
					}
					else if (framecnt >= framemax) {
						if (framemax < 12) {
							framemax++;
						}
						if (cnt >= 12) {
							timing_reset();
						}
						else {
							timing_setcount(cnt - framecnt);
						}
						framereset(0);
					}
				}
				else {
					processwait(waitcnt);
					waitcnt = framecnt;
				}
			}
		}
	}

	sysrunning = 0;

	DestroyWindow(hWnd);

	pccore_cfgupdate();
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
	scrnmng_destroy();
	sysmenu_destroy();

	if (sys_updates	& (SYS_UPDATECFG | SYS_UPDATEOSCFG)) {
		initsave();
	}
	TRACETERM();
	dosio_term();

	return(msg.wParam);

np2main_err3:
	pccore_term();
	S98_trash();
	soundmng_deinitialize();
	scrnmng_destroy();

np2main_err2:
	sysmenu_destroy();

np2main_err1:
	TRACETERM();
	dosio_term();
	return(FALSE);
}

