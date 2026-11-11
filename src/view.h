#pragma once
// Transparent, fully repainted surface. No ListView background, hover cells,
// screen capture, or topmost window is used by the expanded folder.
struct DrawItem {int real=-1;RECT icon{},label{},hit{};HICON image=nullptr;};
struct Expansion {
    HWND window=nullptr,icons=nullptr;bool restore=false,minimized=false;Group group;int page=0,drag=-1,pressed=-1,selected=-1;
    HFONT font=nullptr;RECT work{};POINT down{};int cellW=128,cellH=112,originX=0,originY=0,iconSize=48;
    COLORREF textColor=0;ULONGLONG opened=0;std::vector<DrawItem> drawn;ComPtr<IShellDispatch> shell;
    ~Expansion(){for(auto& item:drawn)if(item.image)DestroyIcon(item.image);if(font)DeleteObject(font);}
};
static std::unique_ptr<Expansion> expanded;
static bool verifying=false;static std::wstring verificationRoot;
static uint64_t frameHash=0;static bool layerUpdated=false;
static void RenderGroup();
static COLORREF TextColor(const Group& g,HWND icons){if(g.colorMode==2)return RGB(0,0,0);if(g.colorMode==3)return RGB(255,255,255);COLORREF c=g.colorMode==1&&icons?ListView_GetTextColor(icons):GetSysColor(COLOR_WINDOWTEXT);return c==CLR_DEFAULT||c==CLR_NONE?GetSysColor(COLOR_WINDOWTEXT):c;}
static void CloseGroup(){if(expanded&&expanded->window)DestroyWindow(expanded->window);}
static int PageCount(){int slot=0;for(auto& item:expanded->group.items)slot=std::max(slot,item.slot);return slot/60+1;}
static int HitItem(POINT point){if(expanded)for(auto& item:expanded->drawn)if(PtInRect(&item.hit,point))return item.real;return -1;}
static void LaunchItem(int real){if(!expanded||real<0||real>=(int)expanded->group.items.size())return;auto path=expanded->group.items[real].path;CloseGroup();if((INT_PTR)ShellExecuteW(nullptr,L"open",path.c_str(),nullptr,DirectoryOf(path).c_str(),SW_SHOWNORMAL)<=32)MessageBoxW(nullptr,L"找不到或無法啟動這個軟體，請確認原始捷徑仍存在。",L"軟體夾",MB_OK|MB_ICONINFORMATION);}
static void AddToOpen(){if(expanded&&PickFiles(expanded->window,expanded->group))RenderGroup();}
static bool VisibleApplications(){bool found=false;EnumWindows([](HWND h,LPARAM p)->BOOL{if(!IsWindowVisible(h)||IsIconic(h)||GetWindow(h,GW_OWNER)||(GetWindowLongPtrW(h,GWL_EXSTYLE)&WS_EX_TOOLWINDOW))return TRUE;wchar_t cls[128];GetClassNameW(h,cls,128);if(!wcscmp(cls,L"Progman")||!wcscmp(cls,L"WorkerW")||!wcscmp(cls,L"Shell_TrayWnd")||!wcscmp(cls,L"Shell_SecondaryTrayWnd"))return TRUE;if(GetWindowLongPtrW(h,GWL_STYLE)&WS_CAPTION){*(bool*)p=true;return FALSE;}return TRUE;},(LPARAM)&found);return found;}
static void ShowMenu(HWND h,POINT point){if(!expanded)return;HMENU menu=CreatePopupMenu(),colors=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"加入軟體…");AppendMenuW(menu,MF_STRING|(expanded->page==0?MF_GRAYED:0),2,L"上一頁");AppendMenuW(menu,MF_STRING|(expanded->page>=PageCount()-1?MF_GRAYED:0),3,L"下一頁");
    AppendMenuW(colors,MF_STRING,20,L"跟隨系統文字色");AppendMenuW(colors,MF_STRING,21,L"跟隨桌面文字色");AppendMenuW(colors,MF_STRING,22,L"黑色");AppendMenuW(colors,MF_STRING,23,L"白色");CheckMenuRadioItem(colors,20,23,20+expanded->group.colorMode,MF_BYCOMMAND);AppendMenuW(menu,MF_POPUP,(UINT_PTR)colors,L"文字顏色");if(expanded->selected>=0)AppendMenuW(menu,MF_STRING,5,L"從軟體夾移除（保留原檔）");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,4,L"收合");
    int command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,h,nullptr);DestroyMenu(menu);if(!expanded)return;
    if(command==1)AddToOpen();else if(command==2&&expanded->page>0){expanded->page--;RenderGroup();}else if(command==3&&expanded->page<PageCount()-1){expanded->page++;RenderGroup();}else if(command==4)CloseGroup();else if(command==5&&expanded->selected>=0){expanded->group.items.erase(expanded->group.items.begin()+expanded->selected);expanded->selected=-1;WriteGroup(expanded->group);RenderGroup();}else if(command>=20&&command<=23){expanded->group.colorMode=command-20;WriteGroup(expanded->group);RenderGroup();}
}
static LRESULT CALLBACK ExpansionProc(HWND h,UINT message,WPARAM w,LPARAM l){
    if(!expanded||expanded->window&&expanded->window!=h)return DefWindowProcW(h,message,w,l);
    switch(message){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT paint;BeginPaint(h,&paint);EndPaint(h,&paint);return 0;}
    case WM_MOUSEMOVE:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(expanded->pressed>=0&&(w&MK_LBUTTON)&&abs(p.x-expanded->down.x)+abs(p.y-expanded->down.y)>GetSystemMetrics(SM_CXDRAG))expanded->drag=expanded->pressed;SetCursor(LoadCursorW(nullptr,expanded->drag>=0?IDC_SIZEALL:HitItem(p)>=0?IDC_HAND:IDC_ARROW));return 0;}
    case WM_LBUTTONDOWN:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};expanded->pressed=HitItem(p);expanded->down=p;expanded->drag=-1;if(expanded->pressed>=0){expanded->selected=expanded->pressed;SetCapture(h);}else if(GetTickCount64()-expanded->opened>GetDoubleClickTime())CloseGroup();return 0;}
    case WM_LBUTTONUP:{int drag=expanded->drag;expanded->pressed=-1;expanded->drag=-1;ReleaseCapture();if(drag>=0){int col=std::clamp((GET_X_LPARAM(l)-expanded->originX)/expanded->cellW,0,9),row=std::clamp((GET_Y_LPARAM(l)-expanded->originY)/expanded->cellH,0,5),target=expanded->page*60+row*10+col,old=expanded->group.items[drag].slot;for(auto& item:expanded->group.items)if(item.slot==target)item.slot=old;expanded->group.items[drag].slot=target;WriteGroup(expanded->group);RenderGroup();}return 0;}
    case WM_LBUTTONDBLCLK:{int item=HitItem({GET_X_LPARAM(l),GET_Y_LPARAM(l)});if(item>=0)LaunchItem(item);return 0;}
    case WM_CONTEXTMENU:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(p.x==-1)GetCursorPos(&p);POINT local=p;ScreenToClient(h,&local);expanded->selected=HitItem(local);ShowMenu(h,p);return 0;}
    case WM_KEYDOWN:if(w==VK_ESCAPE){CloseGroup();return 0;}if(w==VK_RETURN){LaunchItem(expanded->selected);return 0;}if((w==VK_NEXT||w==VK_RIGHT)&&expanded->page<PageCount()-1){expanded->page++;RenderGroup();return 0;}if((w==VK_PRIOR||w==VK_LEFT)&&expanded->page>0){expanded->page--;RenderGroup();return 0;}break;
    case WM_MOUSEWHEEL:if(GET_WHEEL_DELTA_WPARAM(w)<0&&expanded->page<PageCount()-1){expanded->page++;RenderGroup();}else if(GET_WHEEL_DELTA_WPARAM(w)>0&&expanded->page>0){expanded->page--;RenderGroup();}return 0;
    case WM_DROPFILES:{std::vector<std::wstring> paths;HDROP drop=(HDROP)w;for(UINT i=0;i<DragQueryFileW(drop,0xffffffff,nullptr,0);i++){wchar_t p[32768];DragQueryFileW(drop,i,p,32768);paths.emplace_back(p);}DragFinish(drop);AddFiles(expanded->group,paths);WriteGroup(expanded->group);RenderGroup();return 0;}
    case WM_ACTIVATE:if(LOWORD(w)==WA_INACTIVE&&(!l||GetAncestor((HWND)l,GA_ROOTOWNER)!=h)&&GetTickCount64()-expanded->opened>500){CloseGroup();return 0;}break;
    case WM_DESTROY:{auto old=std::move(expanded);if(old->restore&&IsWindow(old->icons))ShowWindow(old->icons,SW_SHOW);if(old->minimized&&old->shell)old->shell->UndoMinimizeALL();objects--;return 0;}
    }
    return DefWindowProcW(h,message,w,l);
}
static bool OpenGroup(const std::wstring& path){
    if(disabled||!Exists(path)||GetPrivateProfileIntW(L"SoftFolder",L"Version",0,path.c_str())!=1)return false;
    if(expanded){if(expanded->group.path==path)return true;CloseGroup();}GraphicsReady();auto data=std::make_unique<Expansion>();data->group=ReadGroup(path);data->icons=DesktopIcons();data->restore=data->icons&&IsWindowVisible(data->icons);data->opened=GetTickCount64();
    POINT cursor{};GetCursorPos(&cursor);MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTOPRIMARY),&monitor);data->work=monitor.rcWork;UINT dpi=data->icons?GetDpiForWindow(data->icons):96;double scale=dpi/96.0;data->iconSize=GetSystemMetricsForDpi(SM_CXICON,dpi);
    LOGFONTW font{};auto desktopFont=data->icons?(HFONT)SendMessageW(data->icons,WM_GETFONT,0,0):nullptr;if(!desktopFont||!GetObjectW(desktopFont,sizeof(font),&font))SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT,sizeof(font),&font,0,dpi);data->font=CreateFontIndirectW(&font);data->textColor=TextColor(data->group,data->icons);
    int width=data->work.right-data->work.left,height=data->work.bottom-data->work.top;data->cellW=std::min((int)(128*scale),width/11);data->cellH=std::min((int)(116*scale),(height-(int)(120*scale))/6);data->originX=(width-data->cellW*10)/2;data->originY=(height-data->cellH*6)/2;
    if(!verifying&&VisibleApplications()){if(FAILED(CoCreateInstance(CLSID_Shell,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&data->shell)))||FAILED(data->shell->MinimizeAll()))return false;data->minimized=true;DwmFlush();}
    if(data->restore)ShowWindow(data->icons,SW_HIDE);
    WNDCLASSW cls{};cls.style=CS_DBLCLKS;cls.hInstance=module;cls.lpszClassName=L"SoftFolders.Native.Expanded.v4";cls.lpfnWndProc=ExpansionProc;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    expanded=std::move(data);objects++;HWND window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_LAYERED,cls.lpszClassName,Stem(path).c_str(),WS_POPUP,expanded->work.left,expanded->work.top,width,height,nullptr,nullptr,module,nullptr);
    if(!window){if(expanded->restore)ShowWindow(expanded->icons,SW_SHOW);if(expanded->minimized)expanded->shell->UndoMinimizeALL();expanded.reset();objects--;return false;}
    expanded->window=window;DragAcceptFiles(window,TRUE);RenderGroup();if(!verifying){ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(window);}return true;
}
static void RenderGroup(){if(!expanded||!expanded->window)return;auto& e=*expanded;e.textColor=TextColor(e.group,e.icons);for(auto& item:e.drawn)if(item.image)DestroyIcon(item.image);e.drawn.clear();int maxSlot=0;for(auto& item:e.group.items)maxSlot=std::max(maxSlot,item.slot);e.page=std::clamp(e.page,0,maxSlot/60);
    int width=e.work.right-e.work.left,height=e.work.bottom-e.work.top;HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;HBITMAP dib=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!dib){DeleteDC(dc);ReleaseDC(nullptr,screen);return;}auto previous=SelectObject(dc,dib);
    {
        Gdiplus::Bitmap canvas(width,height,width*4,PixelFormat32bppPARGB,(BYTE*)pixels);Gdiplus::Graphics graphics(&canvas);graphics.Clear(Gdiplus::Color(1,0,0,0));graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);Gdiplus::Font font(dc,e.font);Gdiplus::SolidBrush ink(Gdiplus::Color(255,GetRValue(e.textColor),GetGValue(e.textColor),GetBValue(e.textColor)));Gdiplus::StringFormat format;format.SetAlignment(Gdiplus::StringAlignmentCenter);format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);float textHeight=font.GetHeight(&graphics)*2.3f;
        for(int real=0;real<(int)e.group.items.size();real++){auto& entry=e.group.items[real];if(entry.slot/60!=e.page)continue;int slot=entry.slot%60,x=e.originX+(slot%10)*e.cellW,y=e.originY+(slot/10)*e.cellH;DrawItem item;item.real=real;int iconX=x+(e.cellW-e.iconSize)/2;item.icon={iconX,y+8,iconX+e.iconSize,y+8+e.iconSize};item.label={x+6,item.icon.bottom+7,x+e.cellW-6,item.icon.bottom+7+(int)std::ceil(textHeight)};item.hit={item.icon.left-6,item.icon.top-4,item.icon.right+6,item.label.bottom};
            if(auto icon=NormalizedIconImage(entry.path))graphics.DrawImage(icon.get(),item.icon.left,item.icon.top,e.iconSize,e.iconSize);auto label=Stem(entry.path);graphics.DrawString(label.c_str(),(int)label.size(),&font,Gdiplus::RectF((float)item.label.left,(float)item.label.top,(float)(item.label.right-item.label.left),(float)(item.label.bottom-item.label.top)),&format,&ink);e.drawn.push_back(item);
        }
        graphics.Flush(Gdiplus::FlushIntentionSync);
        if(verifying&&!verificationRoot.empty()){CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};Gdiplus::Bitmap preview(width,height,PixelFormat32bppARGB);Gdiplus::Graphics background(&preview);background.Clear(Gdiplus::Color(255,242,245,250));background.DrawImage(&canvas,0,0);background.Flush();preview.Save((verificationRoot+L"\\expanded-render.png").c_str(),&png);}
    }
    if(verifying){frameHash=1469598103934665603ull;auto bytes=(const unsigned char*)pixels;for(size_t i=0;i<(size_t)width*height*4;i++)frameHash=(frameHash^bytes[i])*1099511628211ull;}
    POINT position{e.work.left,e.work.top},source{0,0};SIZE size{width,height};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};layerUpdated=UpdateLayeredWindow(e.window,screen,&position,&size,dc,&source,0,&blend,ULW_ALPHA)!=FALSE;
    SelectObject(dc,previous);DeleteObject(dib);DeleteDC(dc);ReleaseDC(nullptr,screen);
}
