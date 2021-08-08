#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <windows.h>


enum runtype { SCR_PREV, SCR_CONFIG, SCR_RUN, SCR_ERRPREV };

char *clsName = "Rechtecke-1.0";
int xmax, ymax;
POINT maus;
int isPreview;
uint8_t *bmp;
HDC hdc, hdcMem;
BITMAP bitmap;
HBITMAP screen;
BITMAPINFO bmi;
HGDIOBJ obj;

LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

void randRect() {
    int x, y, dx, dy;
    uint8_t r,g,b;
    int i, j;

    x=rand()%(xmax-10);
    y=rand()%(ymax-10);
    dx=10+(rand()%90);
    dy=10+(rand()%90);
    if ((x+dx)>=xmax) {
        dx=xmax-x-1;
    }
    if ((y+dy)>=ymax) {
        dy=ymax-y-1;
    }
    r=rand()%0xf0;
    g=rand()%0xf0;
    b=rand()%0xf0;

    for (i=0;i<dx;i++) {
        for (j=0;j<dy;j++) {
            int coord=4*((x+i)+(y+j)*xmax);
            bmp[coord]=bmp[coord]+b;
            bmp[coord+1]=bmp[coord+1]+g;
            bmp[coord+2]=bmp[coord+2]+r;
        }
    }
 
}

enum runtype getRunType(LPSTR args, HWND *hWnd) {
    long long res;

    if (strlen(args)<2) {
        return SCR_RUN;
    }
    if (*args!='/') {
        return SCR_RUN;
    }
    args++;
    switch (*args) {
        case 'c':
        case 'C':
            return SCR_CONFIG;
        
        case 'p':
        case 'P':
            args++;
            if (strlen(args)==0) {
                return SCR_ERRPREV;
            }
            res=0;
            while (*args==' ') {
                args++;
            }
            while (*args!='\0') {
                char c=*args;
                int ziff;
                args++;
                if (c<'0' || c>'9') {
                    return SCR_ERRPREV;
                }
                ziff=c-'0';
                res=10*res+ziff;
            }
            *hWnd=(HWND)res;
            return SCR_PREV;
    }

    return SCR_RUN;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int show) {
    MSG messages;
    WNDCLASSEX wndcl;
    HWND hWnd;
    int e;
    
    wndcl.cbClsExtra=0;
    wndcl.cbSize=sizeof(WNDCLASSEX);
    wndcl.cbWndExtra=0;
    wndcl.hbrBackground=(HBRUSH)CreateSolidBrush(RGB(0,0,0));
    wndcl.hCursor=NULL;
    wndcl.hIcon=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hIconSm=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hInstance=hThisInstance;
    wndcl.lpfnWndProc=wndProc;
    wndcl.lpszClassName=clsName;
    wndcl.lpszMenuName=NULL;
    wndcl.style=0;

    xmax = GetSystemMetrics(SM_CXSCREEN);
    ymax = GetSystemMetrics(SM_CYSCREEN);
    
    if (xmax==0 || ymax==0) {
        MessageBox(NULL, "Konnte Bildschirmdimension nicht abfragen.", "GetSystemMetrics", MB_ICONERROR|MB_OK);
        return -1;
    }

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biHeight=ymax;
    bmi.bmiHeader.biWidth=xmax;
    bmi.bmiHeader.biPlanes=1;
    bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    bmi.bmiHeader.biSizeImage=4*xmax*ymax;

    bmp=malloc(4*xmax*ymax);
    e=0;
    do {
        if ((hdc=GetDC(NULL))==NULL) {
            e=1;
            break;
        }
        if ((hdcMem=CreateCompatibleDC(hdc))==NULL) {
            e=1;
            break;
        }
        if ((screen=CreateCompatibleBitmap(hdc, xmax, ymax))==NULL) {
            e=1;
            break;
        }
        obj=SelectObject(hdcMem, screen);
        if (obj==NULL || obj==HGDI_ERROR) {
            e=1;
            break;
        }
        if (GetObject(screen, sizeof(BITMAP), &bitmap)==0) {
            e=1;
            break;
        }
        if (BitBlt(hdcMem,0,0,bitmap.bmWidth, bitmap.bmHeight, hdc, 0,0, SRCCOPY)==0) {
            e=1;
            break;
        }
        
        if (GetDIBits(hdcMem, screen, 0, bitmap.bmHeight, bmp, &bmi, DIB_RGB_COLORS)==0) {
            e=1;
            break;
        }
        DeleteDC(hdcMem);
    } while (0);

    if (e) {
        MessageBox(0,"Konnte Bildschirminhalt nicht verarbeiten", "Windows GDI", MB_OK|MB_ICONERROR);
        return -1;
    }
    isPreview=0;
    srand(time(NULL));

    switch (getRunType(szCmdLine, &hWnd)) {
        case SCR_RUN:

            if (RegisterClassEx(&wndcl)==0) {
                MessageBox(0, "Konnte Fensterklasse nicht registrieren.", "ReisterClassEx", MB_OK|MB_ICONERROR);
                return 1;
            }

            hWnd=CreateWindowEx(
                WS_EX_APPWINDOW, clsName, NULL, WS_VISIBLE, 0, 0,
                xmax, ymax, HWND_DESKTOP, NULL, hThisInstance,  NULL
            );

            if (hWnd==NULL) {
                MessageBox(NULL, "Fenster konnte nicht erstellt werden.", "CreateWindowEx", MB_ICONERROR|MB_OK);
                return -1;
            }
            
            if (GetCursorPos(&maus)==0) {
                MessageBox(NULL, "Kursorposition konnte nicht ermittelt werden.", "GetCursorPos",MB_ICONERROR|MB_OK);
                return -1;
            }

            if (ShowWindow(hWnd, show)==0) {
                MessageBox(NULL, "Bildschirmschoner konnte nicht hergestellt werden", "ShowWindow", MB_ICONERROR|MB_OK);
                return -1;
            }
            
            break;

        case SCR_PREV: {
            return 0; // kein Preview
        }
        case SCR_CONFIG:
            MessageBox(0,"Dieser Bildschirmschoner ist nicht konfigurierbar.", 0 , 0);
            return 1;

        case SCR_ERRPREV:
            MessageBox(0,"Preview-Modus nur mit Window-Handle möglich.", "Kein Windowhandle", MB_OK|MB_ICONERROR);
            break;
    }

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return 0;
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RECT rct;

    switch (msg) {

        case WM_CREATE: {
            UINT_PTR tid=1;
            DWORD dw;

            dw=GetWindowLong(hWnd, GWL_STYLE);
            dw=dw&(~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX));
            SetWindowLong(hWnd, GWL_STYLE, dw);
            SetTimer(hWnd, tid, 100, NULL);
            break;
        }

        case WM_TIMER: {
            HDC hdc;
            HBRUSH col;
            int x, y;
            int e=0;

            do {
                if ((hdc=GetDC(hWnd))==NULL) {
                    e=1;
                    break;
                }
                if ((hdcMem=CreateCompatibleDC(hdc))==NULL) {
                    e=1;
                    break;
                }
                obj=SelectObject(hdcMem, screen);
                if (obj==NULL || obj==HGDI_ERROR) {
                    e=1;
                    break;
                }
                if (GetObject(screen, sizeof(BITMAP), &bitmap)==0) {
                    e=1;
                    break;
                }
                randRect();
                if (SetDIBits(hdcMem, screen, 0, bitmap.bmHeight, bmp, &bmi, DIB_RGB_COLORS)==0) {
                    e=1;
                    break;
                }
                if (BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0,0, SRCCOPY)==0) {
                    e=1;
                    break;
                }
                DeleteDC(hdcMem);
            } while (0);

            if (e) { // Fehler
                PostQuitMessage(-1);
            }

            break;
        }

        case WM_CLOSE:
            PostQuitMessage(0);
            break;

        //DefScreenSaverProc:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_NCACTIVATE:
            if (wParam==FALSE) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_SETCURSOR:
            SetCursor(NULL);
            return TRUE;


        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            PostQuitMessage(0);
            break;

        case WM_MOUSEMOVE: {
            DWORD lmp=(maus.y<<16)|maus.x;
            if (lmp!=lParam) {
                PostQuitMessage(0);
            }
            return 0;
        }

        case WM_DESTROY:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_SYSCOMMAND:
            if (wParam==SC_CLOSE || wParam==SC_SCREENSAVE) {
                return FALSE;
            }
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}
