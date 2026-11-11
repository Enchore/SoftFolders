#pragma once
static UINT openItemMessage=RegisterWindowMessageW(L"SoftFolders.Native.OpenHitItem.v4");
static std::wstring pendingItem;
static unsigned long clickEvents=0,resolvedClicks=0,openedClicks=0;
static HWND attachedIcons=nullptr;
static ULONGLONG lastDrop=0;
static std::wstring DesktopItemPath(HWND list,int index){
    if(index<0)return L"";
    // Resolve the actual hit item, independently of Explorer's current selection.
    ComPtr<IFolderView2> view;ComPtr<IShellItem> item;PWSTR path=nullptr;
    if(DesktopView(IID_PPV_ARGS(&view))&&SUCCEEDED(view->GetItem(index,IID_PPV_ARGS(&item)))&&item&&SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&path))&&path){std::wstring result=path;CoTaskMemFree(path);return result;}
    // This fallback runs inside Explorer, so LVM_GETITEMTEXT uses local memory.
    DWORD pid=0;GetWindowThreadProcessId(list,&pid);if(pid!=GetCurrentProcessId())return L"";wchar_t name[32768]{};ListView_GetItemText(list,index,0,name,32768);if(!*name)return L"";
    auto desktop=DesktopPath();std::wstring candidate=desktop+L"\\"+name;if(IsGroup(candidate)&&Exists(candidate))return candidate;candidate+=Extension;
    if(Exists(candidate)){for(auto ext:{L".lnk",L".exe",L".url"})if(Exists(desktop+L"\\"+name+ext))return L"";return candidate;}return L"";
}
static void QueueHit(HWND list,int index){clickEvents++;if(GetTickCount64()-lastDrop<GetDoubleClickTime())return;auto path=DesktopItemPath(list,index);if(IsGroup(path)&&Exists(path)){resolvedClicks++;pendingItem=std::move(path);PostMessageW(list,openItemMessage,0,0);}}
static unsigned long ResolveDesktopGroups(HWND list){unsigned long count=0;int items=ListView_GetItemCount(list);for(int i=0;i<items;i++)if(IsGroup(DesktopItemPath(list,i)))count++;return count;}
static POINT desktopDown{};static bool draggingDesktop=false;
static LRESULT CALLBACK DesktopSubclass(HWND h,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
    if(message==statusMessage){if(disabled)return 0;if(w==1)return clickEvents;if(w==2)return resolvedClicks;if(w==3)return openedClicks;if(w==4)return ResolveDesktopGroups(h);return 0x53463a;}
    if(message==stopMessage){disabled=true;integrated=false;CloseGroup();RemoveWindowSubclass(GetParent(h),DesktopSubclass,0x5347);RemoveWindowSubclass(h,DesktopSubclass,0x5346);return 0;}
    if(message==WM_NCDESTROY){RemoveWindowSubclass(h,DesktopSubclass,0x5346);RemoveWindowSubclass(h,DesktopSubclass,0x5347);integrated=false;return DefSubclassProc(h,message,w,l);}
    if(message==openItemMessage){auto path=std::move(pendingItem);pendingItem.clear();if(!disabled&&!path.empty()&&OpenGroup(path))openedClicks++;return 0;}
    if(!disabled){
        if(message==WM_LBUTTONDOWN&&h==attachedIcons){desktopDown={GET_X_LPARAM(l),GET_Y_LPARAM(l)};draggingDesktop=false;}
        if(message==WM_MOUSEMOVE&&h==attachedIcons&&(w&MK_LBUTTON)&&abs(GET_X_LPARAM(l)-desktopDown.x)+abs(GET_Y_LPARAM(l)-desktopDown.y)>GetSystemMetrics(SM_CXDRAG))draggingDesktop=true;
        if(message==WM_LBUTTONUP&&h==attachedIcons&&!draggingDesktop&&!(w&(MK_CONTROL|MK_SHIFT))){LVHITTESTINFO hit{};hit.pt={GET_X_LPARAM(l),GET_Y_LPARAM(l)};int index=ListView_HitTest(h,&hit);auto result=DefSubclassProc(h,message,w,l);if(index>=0&&(hit.flags&(LVHT_ONITEMICON|LVHT_ONITEMLABEL)))QueueHit(h,index);return result;}
        if(message==WM_NOTIFY&&l){auto notification=(NMHDR*)l;if(notification->code==NM_CLICK&&notification->hwndFrom==attachedIcons&&!(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_SHIFT)&0x8000)){auto result=DefSubclassProc(h,message,w,l);QueueHit(notification->hwndFrom,((NMITEMACTIVATE*)l)->iItem);return result;}}
    }
    return DefSubclassProc(h,message,w,l);
}
static bool AttachDesktop(HWND icons){attachedIcons=icons;bool attached=SetWindowSubclass(icons,DesktopSubclass,0x5346,0)!=FALSE;if(attached)SetWindowSubclass(GetParent(icons),DesktopSubclass,0x5347,0);return attached;}
