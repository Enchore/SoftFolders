#pragma once
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <exdisp.h>
#include <shldisp.h>
#include <cmath>
#include <servprov.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <wincodec.h>
#include <psapi.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <set>
#include <atomic>
using Microsoft::WRL::ComPtr;
inline const CLSID CLSID_SoftFolder={0x289b6f42,0x04cb,0x4913,{0x9d,0x26,0x5c,0x36,0x51,0x3b,0x88,0x0a}};
inline constexpr auto ClassId=L"{289B6F42-04CB-4913-9D26-5C36513B880A}";
inline constexpr auto ProgId=L"SoftFolders.Native";
inline constexpr auto Extension=L".softfolder";
struct Entry{std::wstring path;int slot=0;};
struct Group{std::wstring path;std::vector<Entry> items;int colorMode=0;};
inline std::wstring ModulePath(HMODULE module=nullptr){wchar_t p[32768];GetModuleFileNameW(module,p,32768);return p;}
inline std::wstring DirectoryOf(const std::wstring& p){auto n=p.find_last_of(L"\\/");return n==p.npos?L".":p.substr(0,n);}
inline std::wstring Stem(const std::wstring& p){auto n=p.find_last_of(L"\\/");auto s=p.substr(n==p.npos?0:n+1);auto d=s.find_last_of('.');return d==s.npos?s:s.substr(0,d);}
inline bool IsGroup(const std::wstring& p){return _wcsicmp(PathFindExtensionW(p.c_str()),Extension)==0;}
inline bool Exists(const std::wstring& p){return GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES;}
inline std::wstring DesktopPath(){PWSTR p=nullptr;SHGetKnownFolderPath(FOLDERID_Desktop,0,nullptr,&p);std::wstring out=p?p:L"";CoTaskMemFree(p);return out;}
inline std::wstring NewGroupPath(const std::wstring& dir,std::wstring name){for(auto& c:name)if(wcschr(L"<>:\"/\\|?*",c))c=L'_';if(name.empty())name=L"軟體夾";std::wstring p=dir+L"\\"+name+Extension;for(int n=2;Exists(p);n++)p=dir+L"\\"+name+L" ("+std::to_wstring(n)+L")"+Extension;return p;}
inline Group ReadGroup(const std::wstring& path){
    Group g;g.path=path;int version=GetPrivateProfileIntW(L"SoftFolder",L"Version",0,path.c_str());if(version!=1)return g;
    g.colorMode=std::clamp((int)GetPrivateProfileIntW(L"SoftFolder",L"ColorMode",0,path.c_str()),0,3);
    int count=std::min(6000,(int)GetPrivateProfileIntW(L"SoftFolder",L"Count",0,path.c_str()));std::set<int> used;
    for(int n=0;n<count;n++){wchar_t p[32768];auto key=std::to_wstring(n);GetPrivateProfileStringW(L"Paths",key.c_str(),L"",p,32768,path.c_str());if(!*p)continue;int slot=GetPrivateProfileIntW(L"Slots",key.c_str(),n,path.c_str());if(slot<0||slot>60000||used.count(slot)){slot=0;while(used.count(slot))slot++;}used.insert(slot);g.items.push_back({p,slot});}return g;
}
inline bool WriteGroup(const Group& g){
    std::wstring data=L"\xFEFF[SoftFolder]\r\nVersion=1\r\nColorMode="+std::to_wstring(g.colorMode)+L"\r\nCount="+std::to_wstring(g.items.size())+L"\r\n\r\n[Paths]\r\n";
    for(size_t i=0;i<g.items.size();i++)data+=std::to_wstring(i)+L"="+g.items[i].path+L"\r\n";
    data+=L"\r\n[Slots]\r\n";for(size_t i=0;i<g.items.size();i++)data+=std::to_wstring(i)+L"="+std::to_wstring(g.items[i].slot)+L"\r\n";
    auto temp=g.path+L".tmp";HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;DWORD wrote=0;bool ok=WriteFile(h,data.data(),(DWORD)data.size()*2,&wrote,nullptr)&&wrote==data.size()*2;FlushFileBuffers(h);CloseHandle(h);
    if(ok)ok=MoveFileExW(temp.c_str(),g.path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;if(!ok)DeleteFileW(temp.c_str());
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,g.path.c_str());if(ok)SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATHW,g.path.c_str(),nullptr);return ok;
}
inline bool Supported(const std::wstring& p){auto e=PathFindExtensionW(p.c_str());return !_wcsicmp(e,L".lnk")||!_wcsicmp(e,L".exe")||!_wcsicmp(e,L".url")||!_wcsicmp(e,L".appref-ms");}
inline int AddFiles(Group& g,const std::vector<std::wstring>& files){int added=0;
    for(auto& p:files){DWORD attr=GetFileAttributesW(p.c_str());if(attr==INVALID_FILE_ATTRIBUTES)continue;if(attr&FILE_ATTRIBUTE_DIRECTORY){WIN32_FIND_DATAW f;HANDLE h=FindFirstFileW((p+L"\\*").c_str(),&f);if(h!=INVALID_HANDLE_VALUE){std::vector<std::wstring> children;do{if(!(f.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))children.push_back(p+L"\\"+f.cFileName);}while(FindNextFileW(h,&f));FindClose(h);added+=AddFiles(g,children);}continue;}
        if(!Supported(p)||g.items.size()>=6000||std::any_of(g.items.begin(),g.items.end(),[&](auto& e){return !_wcsicmp(e.path.c_str(),p.c_str());}))continue;int slot=0;while(std::any_of(g.items.begin(),g.items.end(),[&](auto& e){return e.slot==slot;}))slot++;g.items.push_back({p,slot});added++;}return added;
}
inline std::vector<std::wstring> FromData(IDataObject* object){std::vector<std::wstring> files;FORMATETC fmt={CF_HDROP,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};STGMEDIUM st{};if(object&&SUCCEEDED(object->GetData(&fmt,&st))){auto drop=(HDROP)st.hGlobal;if(drop){UINT count=DragQueryFileW(drop,0xffffffff,nullptr,0);for(UINT i=0;i<count;i++){wchar_t p[32768];DragQueryFileW(drop,i,p,32768);files.emplace_back(p);}}ReleaseStgMedium(&st);}return files;}
inline bool PickFiles(HWND owner,Group& group){std::vector<wchar_t> buf(65536);OPENFILENAMEW ofn={sizeof(ofn)};ofn.hwndOwner=owner;ofn.lpstrFilter=L"軟體與捷徑\0*.lnk;*.exe;*.url;*.appref-ms\0\0";ofn.lpstrFile=buf.data();ofn.nMaxFile=(DWORD)buf.size();ofn.Flags=OFN_EXPLORER|OFN_ALLOWMULTISELECT|OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
    if(!GetOpenFileNameW(&ofn))return false;std::vector<std::wstring> paths;std::wstring first=buf.data();auto p=buf.data()+first.size()+1;if(!*p)paths.push_back(first);else for(;*p;p+=wcslen(p)+1)paths.push_back(first+L"\\"+p);AddFiles(group,paths);return WriteGroup(group);}
inline HICON FileIcon(const std::wstring& path,int size){SHFILEINFOW info{};if(SHGetFileInfoW(path.c_str(),0,&info,sizeof(info),SHGFI_SYSICONINDEX)){ComPtr<IImageList> list;int kind=size>48?SHIL_JUMBO:size>32?SHIL_EXTRALARGE:SHIL_LARGE;if(SUCCEEDED(SHGetImageList(kind,IID_PPV_ARGS(&list)))){HICON icon=nullptr;if(SUCCEEDED(list->GetIcon(info.iIcon,ILD_TRANSPARENT,&icon)))return icon;}}SHGetFileInfoW(path.c_str(),0,&info,sizeof(info),SHGFI_ICON|SHGFI_LARGEICON);return info.hIcon;}
inline std::unique_ptr<Gdiplus::Bitmap> IconImage(HICON icon){ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmap> source;ComPtr<IWICFormatConverter> converter;UINT width=0,height=0;
    if(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))&&SUCCEEDED(factory->CreateBitmapFromHICON(icon,&source))&&SUCCEEDED(source->GetSize(&width,&height))&&SUCCEEDED(factory->CreateFormatConverter(&converter))&&SUCCEEDED(converter->Initialize(source.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))){
        auto bitmap=std::make_unique<Gdiplus::Bitmap>((int)width,(int)height,PixelFormat32bppPARGB);Gdiplus::Rect rect(0,0,width,height);Gdiplus::BitmapData data{};if(bitmap->LockBits(&rect,Gdiplus::ImageLockModeWrite,PixelFormat32bppPARGB,&data)==Gdiplus::Ok){bool ok=data.Stride>0&&SUCCEEDED(converter->CopyPixels(nullptr,(UINT)data.Stride,(UINT)data.Stride*height,(BYTE*)data.Scan0));bitmap->UnlockBits(&data);if(ok)return bitmap;}}
    return std::make_unique<Gdiplus::Bitmap>(icon);
}
#include "normalized_icons.h"
inline HICON GroupIcon(const Group& g,int size){size=std::clamp(size,16,256);Gdiplus::Bitmap bitmap(size,size,PixelFormat32bppARGB);Gdiplus::Graphics graphics(&bitmap);graphics.Clear(Gdiplus::Color(0,0,0,0));graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    float r=size*.2f,d=r*2,s=(float)size-1;Gdiplus::GraphicsPath shape;shape.AddArc(0.f,0.f,d,d,180.f,90.f);shape.AddArc(s-d,0.f,d,d,270.f,90.f);shape.AddArc(s-d,s-d,d,d,0.f,90.f);shape.AddArc(0.f,s-d,d,d,90.f,90.f);shape.CloseFigure();Gdiplus::SolidBrush brush(Gdiplus::Color(255,49,63,84));graphics.FillPath(&brush,&shape);
    auto sorted=g.items;std::sort(sorted.begin(),sorted.end(),[](auto&a,auto&b){return a.slot<b.slot;});int count=std::min(9,(int)sorted.size()),columns=3,rows=3,pad=std::max(2,(int)std::round(size*.11)),gap=std::max(1,(int)std::round(size*.05)),cell=std::max(1,(size-pad*2-gap*(columns-1))/columns),baseY=(size-(rows*cell+(rows-1)*gap))/2;
    for(int n=0;n<count;n++){int row=n/columns,baseX=(size-(columns*cell+(columns-1)*gap))/2;auto image=NormalizedIconImage(sorted[n].path);if(image)graphics.DrawImage(image.get(),Gdiplus::Rect(baseX+(n%columns)*(cell+gap),baseY+row*(cell+gap),cell,cell),16,16,224,224,Gdiplus::UnitPixel);}HICON out=nullptr;bitmap.GetHICON(&out);return out;
}
inline bool DesktopView(REFIID iid,void** out){if(!out)return false;*out=nullptr;ComPtr<IShellWindows> windows;ComPtr<IDispatch> dispatch;ComPtr<IServiceProvider> service;ComPtr<IShellBrowser> browser;ComPtr<IShellView> view;VARIANT location{},root{};location.vt=VT_I4;location.lVal=CSIDL_DESKTOP;long hwnd=0;
    return SUCCEEDED(CoCreateInstance(CLSID_ShellWindows,nullptr,CLSCTX_LOCAL_SERVER,IID_PPV_ARGS(&windows)))&&windows&&windows->FindWindowSW(&location,&root,SWC_DESKTOP,&hwnd,SWFO_NEEDDISPATCH,&dispatch)==S_OK&&dispatch&&SUCCEEDED(dispatch.As(&service))&&service&&SUCCEEDED(service->QueryService(SID_STopLevelBrowser,IID_PPV_ARGS(&browser)))&&browser&&SUCCEEDED(browser->QueryActiveShellView(&view))&&view&&SUCCEEDED(view->QueryInterface(iid,out));}
inline HWND DesktopIcons(){HWND progman=FindWindowW(L"Progman",nullptr);HWND view=FindWindowExW(progman,nullptr,L"SHELLDLL_DefView",nullptr);HWND primary=view?FindWindowExW(view,nullptr,L"SysListView32",nullptr):nullptr;if(primary&&IsWindowVisible(view))return primary;
    struct Search{HWND visible=nullptr,fallback=nullptr;} search;search.fallback=primary;
    EnumWindows([](HWND h,LPARAM p)->BOOL{wchar_t cls[64];GetClassNameW(h,cls,64);if(wcscmp(cls,L"WorkerW")&&wcscmp(cls,L"Progman"))return TRUE;HWND v=FindWindowExW(h,nullptr,L"SHELLDLL_DefView",nullptr);HWND icons=v?FindWindowExW(v,nullptr,L"SysListView32",nullptr):nullptr;if(!icons)return TRUE;auto& s=*(Search*)p;if(!s.fallback)s.fallback=icons;if(IsWindowVisible(v)){s.visible=icons;return FALSE;}return TRUE;},(LPARAM)&search);return search.visible?search.visible:search.fallback;}
inline std::wstring SelectedDesktopFile(){ComPtr<IShellView> view;ComPtr<IDataObject> data;if(DesktopView(IID_PPV_ARGS(&view))&&SUCCEEDED(view->GetItemObject(SVGIO_SELECTION,IID_PPV_ARGS(&data)))){auto files=FromData(data.Get());if(files.size()==1)return files[0];}return L"";}
