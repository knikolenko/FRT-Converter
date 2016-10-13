#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "controls.h"
//#include "ieee754.h"

#define WM_TRAY_CLICK (WM_USER + 200)

static const CHAR WindowTitle[] = "FRT Converter 1.04";

UINT wnd_xpos = 0;
UINT wnd_ypos = 0;
UINT key_hook = VK_CAPITAL;
UINT g_traymode = 0;

BOOL bVisible = TRUE;

const char cfg_fname[] = "./converter.ini";

HHOOK hHook = NULL;

char buf[100] = {0};

BOOL redraw = FALSE;

HWND hDlgMain = NULL;

HWND hEditUInt8     = NULL;
HWND hEditUInt16    = NULL;
HWND hEditUInt32    = NULL;
HWND hEditUInt64    = NULL;
HWND hEditInt8      = NULL;
HWND hEditInt16     = NULL;
HWND hEditInt32     = NULL;
HWND hEditInt64     = NULL;
HWND hEditHex       = NULL;
HWND hEditFloat     = NULL;
HWND hEditReal      = NULL;
HWND hEditDouble    = NULL;
HWND hEditWinTime   = NULL;
HWND hEditCTime     = NULL;
HWND hEditIeee754   = NULL;
HWND hBtnReverseHex = NULL;

void SetUInt8 (uint8_t *val) {
	char outbuf[4] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%u", *val);
	Edit_SetText(hEditUInt8, outbuf);
}
void SetUInt16 (uint16_t *val) {
	char outbuf[6] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%u", *val);
	Edit_SetText(hEditUInt16, outbuf);
}
void SetUInt32 (uint32_t *val) {
	char outbuf[30] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%u", *val);
	Edit_SetText(hEditUInt32, outbuf);
}
void SetUInt64 (unsigned long long *val) {
	char outbuf[50] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%llu", *val);
	Edit_SetText(hEditUInt64, outbuf);
}
void SetInt8 (int8_t *val) {
	char outbuf[5] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%d", *val);
	Edit_SetText(hEditInt8, outbuf);
}
void SetInt16 (int16_t *val) {
	char outbuf[7] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%d", *val);
	Edit_SetText(hEditInt16, outbuf);
}
void SetInt32 (int32_t *val) {
	char outbuf[30] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%d", *val);
	Edit_SetText(hEditInt32, outbuf);
}
void SetInt64 (long long int *val) {
	char outbuf[50] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%lld", *val);
	Edit_SetText(hEditInt64, outbuf);
}
void SetHex (UCHAR *val) {
	char outbuf[30] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%02X%02X%02X%02X%02X%02X%02X%02X", 
		val[0], val[1], val[2], val[3], 
		val[4], val[5], val[6], val[7]
	);
	Edit_SetText(hEditHex, outbuf);
}
void SetFloat (float *val) {
	char outbuf[32] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%f", *val);
	Edit_SetText(hEditFloat, outbuf);
}
void SetDouble (double *val) {
	char outbuf[30] = {0};
	
	snprintf(outbuf, sizeof(outbuf), "%e", *val);
	Edit_SetText(hEditDouble, outbuf);
}
void SetWinTime (FILETIME *val) {
	SYSTEMTIME sys = {0};
	char outbuf[30] = {0};

	FileTimeToSystemTime(val, &sys);
	snprintf(outbuf, sizeof(outbuf), "%02d.%02d.%04d %02d:%02d:%02d", 
		sys.wDay,  sys.wMonth,  sys.wYear, 
		sys.wHour, sys.wMinute, sys.wSecond
	);
	Edit_SetText(hEditWinTime, outbuf);
}
void SetCTime (time_t *val) {
	struct tm *tmval = gmtime(val);
	char outbuf[30] = {0};

	strftime(outbuf, sizeof(outbuf), "%d.%m.%Y %T", tmval);
	Edit_SetText(hEditCTime, outbuf);
}

void UpdateAll (UCHAR *buf)
{
	if (redraw) return;
	redraw = TRUE;

	HWND hWndFocus = GetFocus();
	// модифицируем все поля кроме активного
	if (hWndFocus != hEditUInt8  ) SetUInt8  ((uint8_t *)          buf); 
	if (hWndFocus != hEditUInt16 ) SetUInt16 ((uint16_t *)         buf);
	if (hWndFocus != hEditUInt32 ) SetUInt32 ((uint32_t *)         buf);
	if (hWndFocus != hEditUInt64 ) SetUInt64 ((unsigned long long*)buf);
	if (hWndFocus != hEditInt8   ) SetInt8   ((int8_t *)           buf);
	if (hWndFocus != hEditInt16  ) SetInt16  ((int16_t *)          buf);
	if (hWndFocus != hEditInt32  ) SetInt32  ((int32_t *)          buf);
	if (hWndFocus != hEditInt64  ) SetInt64  ((long long int*)     buf);
	if (hWndFocus != hEditHex    ) SetHex    (                     buf);
	if (hWndFocus != hEditFloat  ) SetFloat  ((float *)            buf);
	if (hWndFocus != hEditDouble ) SetDouble ((double *)           buf);
	if (hWndFocus != hEditWinTime) SetWinTime((FILETIME *)         buf);
	if (hWndFocus != hEditCTime  ) SetCTime  ((time_t *)           buf);

	redraw = FALSE;
}

int get_utc_offset(void) {
  time_t zero = 24*60*60L;
  struct tm * timeptr;
  int gmtime_hours;

  /* get the local time for Jan 2, 1900 00:00 UTC */
  timeptr = localtime( &zero );
  gmtime_hours = timeptr->tm_hour;

  /* if the local time is the "day before" the UTC, subtract 24 hours
    from the hours to get the UTC offset */
  if( timeptr->tm_mday < 2 )
    gmtime_hours -= 24;

  return gmtime_hours;
}

/*
  the utc analogue of mktime,
  (much like timegm on some systems)
*/
time_t tm_to_time_t_utc( struct tm * timeptr ) {
  /* gets the epoch time relative to the local time zone,
  and then adds the appropriate number of seconds to make it UTC */
  return mktime( timeptr ) + get_utc_offset() * 3600;
}

void TrayAdd(void)
{
	NOTIFYICONDATA nid = {0};

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd   = hDlgMain;
	nid.uID    = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAY_CLICK;
	nid.hIcon  = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(8001), IMAGE_ICON, 16, 16, LR_SHARED);

	//lstrcpy(nid.szTip, "FRT Converter");
	strncpy(nid.szTip, WindowTitle, sizeof(nid.szTip));

	Shell_NotifyIcon(NIM_ADD, &nid);

}

void TrayDelete(void)
{
	NOTIFYICONDATA nid = {0};

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd   = hDlgMain;
	nid.uID    = 1;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void Hide(void)
{
	if (g_traymode == 1) {
		TrayAdd();
	}

	ShowWindow(hDlgMain, SW_HIDE);
	bVisible = FALSE;
}

void Show(void)
{
	CHAR buf[100];

	if (g_traymode == 1) {
		TrayDelete();
	}
	
	// при вызове из расширенного списка трея, скрываем само выпадающее окно трея
	GetClassName(GetForegroundWindow(), buf, sizeof(buf));
	if (!strcmp(buf, "NotifyIconOverflowWindow")) {
		SendMessage(GetForegroundWindow(), WM_CLOSE, 0, 0);
	}
	// Показываем свое окно
	ShowWindow(hDlgMain, SW_SHOW);
	bVisible = TRUE;
}

void OnInitDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rc;

	hDlgMain       = hwndDlg;
	hEditUInt8     = GetDlgItem(hwndDlg, EDIT_UINT8);
	hEditUInt16    = GetDlgItem(hwndDlg, EDIT_UINT16);
	hEditUInt32    = GetDlgItem(hwndDlg, EDIT_UINT32);
	hEditUInt64    = GetDlgItem(hwndDlg, EDIT_UINT64);
	hEditInt8      = GetDlgItem(hwndDlg, EDIT_INT8);
	hEditInt16     = GetDlgItem(hwndDlg, EDIT_INT16);
	hEditInt32     = GetDlgItem(hwndDlg, EDIT_INT32);
	hEditInt64     = GetDlgItem(hwndDlg, EDIT_INT64);
	hEditHex       = GetDlgItem(hwndDlg, EDIT_HEX);
	hEditFloat     = GetDlgItem(hwndDlg, EDIT_FLOAT);
	hEditDouble    = GetDlgItem(hwndDlg, EDIT_DOUBLE);
	hEditWinTime   = GetDlgItem(hwndDlg, EDIT_WINTIME);
	hEditCTime     = GetDlgItem(hwndDlg, EDIT_CTIME);
	//hEditIeee754 = GetDlgItem(hwndDlg, EDIT_IEEE754);
	hBtnReverseHex = GetDlgItem(hwndDlg, BTN_REVHEX);

	SetWindowText(hwndDlg, WindowTitle);

	Edit_LimitText(hEditUInt8,  3);
	Edit_LimitText(hEditUInt16, 5);
	Edit_LimitText(hEditUInt32, 10);
	Edit_LimitText(hEditInt8,   4);
	Edit_LimitText(hEditInt16,  6);
	Edit_LimitText(hEditInt32,  11);
	Edit_LimitText(hEditHex,    8*2);

	// задаем стартовое значение
	SetFocus(hEditUInt8);
	Edit_SetText(hEditUInt8, "0");

	//SendMessage(hwndDlg, WM_COMMAND, EN_CHANGE << 16 | EDIT_UINT8, 0);
	// восстанавливаем положение окна из настроек
	GetWindowRect(hwndDlg, &rc);
	MoveWindow(hwndDlg, wnd_xpos, wnd_ypos, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	if (g_traymode == 2)
	{
		TrayAdd();
	}
}

void OnPressReverseHex(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHAR tmpbuf[sizeof(buf)] = { 0 };
	memset(buf, 0, sizeof(buf));
	Edit_GetText(hEditHex, buf, sizeof(buf) - 1);
	size_t len = strlen(buf);
	if (len <= 2)
		return;
	if (len & 1)
	{
		snprintf(tmpbuf, sizeof(tmpbuf) - 1, "0%s", buf);
		len++;
	}
	else
	{
		snprintf(tmpbuf, sizeof(tmpbuf) - 1, "%s", buf);
	}

	memset(buf, 0, sizeof(buf));
	for (size_t i = 0; i < len; i += 2)
	{
		memcpy(&buf[i], &tmpbuf[len - i - 2], 2);
	}

	SetFocus(hEditHex);
	Edit_SetText(hEditHex, buf);
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			OnInitDialog(hwndDlg, uMsg, wParam, lParam);
			break;
		}
		case WM_TRAY_CLICK: {
			// событие от иконки из трея
			if (LOWORD(lParam) == WM_LBUTTONUP) {
				if (bVisible) {
					Hide();
				} else {
					Show();
				}
			}
			break;
		}
		case WM_CLOSE : {
			// сохраняем положение окна
			RECT rc;
			CHAR buf[32] = {0};
			
			GetWindowRect(hwndDlg, &rc);
			snprintf(buf, sizeof(buf)-1, "%u", rc.left);
			WritePrivateProfileString("main", "xpos", buf, cfg_fname);
			snprintf(buf, sizeof(buf)-1, "%u", rc.top);
			WritePrivateProfileString("main", "ypos", buf, cfg_fname);

			UnhookWindowsHookEx(hHook);

			TrayDelete();
			EndDialog(hwndDlg, 0);
			break;
		}
		// даблклик по форме
		case WM_LBUTTONDBLCLK: {
			Hide();
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wParam) == BTN_REVHEX)
			{
				OnPressReverseHex(hwndDlg, uMsg, wParam, lParam);
				break;
			}
			// произошло изменение одного из editbox'ов
			if (HIWORD(wParam) == EN_CHANGE) {
				memset(buf, 0, sizeof(buf));
				switch (LOWORD(wParam)) {
					case EDIT_UINT8: {
						if (GetFocus() != hEditUInt8) break;
						Edit_GetText(hEditUInt8, buf, sizeof(buf)-1);
						UCHAR outbuf[16] = {0};
						*(ULONG *)outbuf = atol(buf) & 0xFF;
						UpdateAll(outbuf);
						break;
					}
					case EDIT_UINT16: {
						if (GetFocus() != hEditUInt16) break;
						Edit_GetText(hEditUInt16, buf, sizeof(buf)-1);
						UCHAR outbuf[16] = {0};
						*(ULONG *)outbuf = atol(buf) & 0xFFFF;
						UpdateAll(outbuf);
						break;
					}
					case EDIT_UINT32: {
						if (GetFocus() != hEditUInt32) break;
						Edit_GetText(hEditUInt32, buf, sizeof(buf)-1);
						UCHAR outbuf[16] = {0};
						*(ULONG *)outbuf = atol(buf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_UINT64: {
						if (GetFocus() != hEditUInt64) break;
						Edit_GetText(hEditUInt64, buf, sizeof(buf)-1);
						UCHAR outbuf[16] = {0};
						sscanf(buf, "%llu", (unsigned long long int *)&outbuf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_INT8: {
						if (GetFocus() != hEditInt8) break;
						Edit_GetText(hEditInt8, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						*(int8_t *)outbuf = atoi(buf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_INT16: {
						if (GetFocus() != hEditInt16) break;
						Edit_GetText(hEditInt16, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						*(int16_t *)outbuf = atoi(buf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_INT32: {
						if (GetFocus() != hEditInt32) break;
						Edit_GetText(hEditInt32, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						*(int32_t *)outbuf = atoi(buf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_INT64: {
						if (GetFocus() != hEditInt64) break;
						Edit_GetText(hEditInt64, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						sscanf(buf, "%lld", (long long int *)&outbuf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_HEX: {
						if (GetFocus() != hEditHex) break;
						UCHAR tmpbuf[sizeof(buf)] = {0};
						Edit_GetText(hEditHex, (char *)tmpbuf, sizeof(tmpbuf)-1);
						size_t len = strlen((const char*)tmpbuf);

						size_t lastchar = 0;
						memset(buf, 0, sizeof(buf));
						for (size_t i = 0; i < len; i++) {
							if (isxdigit(tmpbuf[i])) buf[lastchar++] = tmpbuf[i];
						}
						buf[lastchar] = '\x0';
						if (lastchar != len) {
							Edit_SetText(hEditHex, buf);
							SendMessage( hEditHex, EM_SETSEL, len, len );
							break;
						}

						len = len >> 1;
						UCHAR outbuf[32] = {0};
						for (size_t i = 0; i < len; i++)
							sscanf(&buf[i*2], "%02hhX", &outbuf[i]);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_FLOAT: {
						if (GetFocus() != hEditFloat) break;
						Edit_GetText(hEditFloat, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						sscanf(buf, "%f", (float *)&outbuf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_DOUBLE: {
						if (GetFocus() != hEditDouble) break;
						Edit_GetText(hEditDouble, buf, sizeof(buf)-1);
						UCHAR outbuf[32] = {0};
						sscanf(buf, "%e", (float *)&outbuf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_WINTIME: {
						if (GetFocus() != hEditWinTime) break;
						Edit_GetText(hEditWinTime, buf, sizeof(buf)-1);
						SYSTEMTIME sys = {0};
						WORD month = 0;
						sscanf(buf, "%02hu.%02hu.%04hu %02hu:%02hu:%02hu", &sys.wDay, &month, &sys.wYear, &sys.wHour, &sys.wMinute, &sys.wSecond);
						sys.wMonth = month;
						UCHAR outbuf[32] = {0};
						SystemTimeToFileTime(&sys, (LPFILETIME)&outbuf);
						UpdateAll(outbuf);
						break;
					}
					case EDIT_CTIME: {
						if (GetFocus() != hEditCTime) break;
						Edit_GetText(hEditCTime, buf, sizeof(buf)-1);
						time_t rawtime;
						time ( &rawtime );
						struct tm *t = localtime ( &rawtime );
						sscanf(buf, "%02u.%02u.%04u %02u:%02u:%02u", &t->tm_mday, &t->tm_mon, &t->tm_year, &t->tm_hour, &t->tm_min, &t->tm_sec);
						t->tm_year -= 1900;
						if (t->tm_mon > 0)
							t->tm_mon -= 1;
						else {
							t->tm_year -= 1;
							t->tm_mon = 11;
						}
						UCHAR outbuf[32] = {0};
						*(time_t *)outbuf = tm_to_time_t_utc(t);
						UpdateAll(outbuf);
						break;
					}
				} // switch
			} // EN_CHANGE
			break;
		}
		default:
			return 0;
			//return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
	}
	return 1;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT param = (PKBDLLHOOKSTRUCT)lParam;
	if (!nCode && param->vkCode == key_hook && param->flags&LLKHF_UP) {
		if (bVisible) {
			Hide();
		} else {
			Show();
		}
		return 1;
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CHAR szkey[32] = {0};

	InitCommonControls();

	wnd_xpos = (GetSystemMetrics(SM_CXSCREEN) >> 1) - 70; // w/2
	wnd_ypos = (GetSystemMetrics(SM_CYSCREEN) >> 1) - 108; // h/2

	wnd_xpos   = GetPrivateProfileInt("main", "xpos",     wnd_xpos, cfg_fname);
	wnd_ypos   = GetPrivateProfileInt("main", "ypos",     wnd_ypos, cfg_fname);
	g_traymode = GetPrivateProfileInt("main", "trayMode", 2,        cfg_fname);

	GetPrivateProfileString("main", "key", "0x14", szkey, sizeof(szkey), cfg_fname);
	
	key_hook = strtol(szkey, 0, 16);
	//sscanf(szkey, "0x%x", &key_hook);

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, hInstance, 0);
	DialogBox(hInstance, (LPSTR)1001, NULL, &DialogProc);
	return 0;
}
