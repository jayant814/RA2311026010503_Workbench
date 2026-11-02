#include <windows.h>
#include "core/core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <commdlg.h>
#include <functional>
#include <iomanip>
#include <shellapi.h>
#include <sstream>
#include <thread>
#include <windowsx.h>
namespace {
using namespace tb;
// ── Premium Dark Engineering Palette ──────────────────────────────────────────
constexpr COLORREF C_BG       = RGB(16,18,22);    // #101216 deep obsidian
constexpr COLORREF C_BG2      = RGB(23,26,32);    // #171A20 secondary
constexpr COLORREF C_PANEL    = RGB(29,34,42);     // #1D222A panel
constexpr COLORREF C_ELEV     = RGB(37,43,52);     // #252B34 elevated
constexpr COLORREF C_BORDER   = RGB(50,56,68);     // subtle border
constexpr COLORREF C_TXT      = RGB(243,245,247);  // primary text
constexpr COLORREF C_TXT2     = RGB(161,169,181);  // secondary text
constexpr COLORREF C_TXT3     = RGB(115,125,139);  // muted text
constexpr COLORREF C_CYAN     = RGB(85,199,232);   // #55C7E8 electric cyan
constexpr COLORREF C_BLUE     = RGB(75,141,255);   // #4B8DFF engineering blue
constexpr COLORREF C_GREEN    = RGB(69,196,134);   // #45C486 success
constexpr COLORREF C_WARN     = RGB(233,180,91);   // #E9B45B warning
constexpr COLORREF C_ERR      = RGB(227,107,107);  // #E36B6B error
constexpr COLORREF C_SIDEBAR  = RGB(20,22,28);     // sidebar bg
constexpr COLORREF C_SIDEBAR_H= RGB(37,43,52);     // sidebar hover
constexpr COLORREF C_SIDEBAR_A= RGB(29,34,42);     // sidebar active
constexpr COLORREF C_BTN      = RGB(37,43,52);     // button bg
constexpr COLORREF C_BTN_H    = RGB(50,56,68);     // button hover
constexpr COLORREF C_BTN_P    = RGB(85,199,232);   // button primary
constexpr COLORREF C_GRID     = RGB(37,43,52);     // chart grid
constexpr int SIDEBAR_W = 200;
constexpr int TOPBAR_H  = 48;
constexpr int CTRL_ROW  = 8;
HWND g_main, g_canvas, g_status;
HFONT f_title, f_ui, f_sm, f_xs, f_mono;
HBRUSH br_bg, br_panel, br_sidebar;
int g_view = 0;
std::vector<HWND> g_ctrls;
std::vector<std::pair<int,HWND>> g_fields;
std::filesystem::path g_root;
std::thread g_job;
std::atomic<bool> g_cancel{false};
std::atomic<int> g_progress{0};
bool g_busy = false;
std::function<void()> g_completion;
std::string g_error, g_notice = "Ready";
std::vector<Triangle> g_tris;
std::vector<Node> g_nodes;
std::vector<Timing> g_timings;
Rod g_rod;
Solution g_sol;
std::vector<Device> g_devs = discover();
int g_devi = 0, g_bcnt = 1000, g_bmesh = 4000;
bool g_cloud = false, g_drag = false, g_pan = false;
POINT g_prev{};
double g_yaw=.6, g_pitch=.35, g_zoom=1, g_px=0, g_py=0;
std::chrono::steady_clock::time_point g_start = std::chrono::steady_clock::now();
bool g_ready = false;
// ── Helpers ───────────────────────────────────────────────────────────────────
std::wstring W(const std::string &s) {
    if(s.empty()) return {};
    int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,0,0);
    std::wstring w(n,0); MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,w.data(),n);
    w.pop_back(); return w;
}
std::string N(const std::wstring &w) {
    int n=WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,0,0,0,0);
    std::string s(n,0); WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,s.data(),n,0,0);
    s.pop_back(); return s;
}
std::string F(double x,int p=5){std::ostringstream s;s<<std::setprecision(p)<<x;return s.str();}
void setStatus(const std::string &s){g_notice=s; SetWindowTextW(g_status,W(s).c_str());}
void redraw(){InvalidateRect(g_canvas,nullptr,FALSE);}
// ── GDI primitives ────────────────────────────────────────────────────────────
void drawText(HDC dc,int x,int y,const std::string &s,COLORREF c=C_TXT,HFONT f=nullptr){
    if(f)SelectObject(dc,f); SetTextColor(dc,c); SetBkMode(dc,TRANSPARENT);
    auto w=W(s); TextOutW(dc,x,y,w.c_str(),(int)w.size());
}
void drawLine(HDC dc,int x1,int y1,int x2,int y2,COLORREF c,int w=1){
    HPEN p=CreatePen(PS_SOLID,w,c); auto o=SelectObject(dc,p);
    MoveToEx(dc,x1,y1,0); LineTo(dc,x2,y2); SelectObject(dc,o); DeleteObject(p);
}
void fillRect(HDC dc,RECT r,COLORREF c){
    HBRUSH b=CreateSolidBrush(c); FillRect(dc,&r,b); DeleteObject(b);
}
void fillRound(HDC dc,RECT r,COLORREF c,int rx=4){
    HBRUSH b=CreateSolidBrush(c); HPEN p=CreatePen(PS_SOLID,0,c);
    auto ob=SelectObject(dc,b),op=SelectObject(dc,p);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,rx,rx);
    SelectObject(dc,ob);SelectObject(dc,op); DeleteObject(b);DeleteObject(p);
}
void borderRect(HDC dc,RECT r,COLORREF c,int w=1){
    HPEN p=CreatePen(PS_SOLID,w,c); HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
    auto op=SelectObject(dc,p),ob=SelectObject(dc,nb);
    Rectangle(dc,r.left,r.top,r.right,r.bottom);
    SelectObject(dc,op);SelectObject(dc,ob); DeleteObject(p);
}
// ── Control factory ───────────────────────────────────────────────────────────
void mkBtn(const std::string &lbl,int x,int y,int w,int id,bool primary=false){
    auto c=CreateWindowW(L"BUTTON",W(lbl).c_str(),
        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|WS_TABSTOP,x,y,w,30,g_main,
        (HMENU)(INT_PTR)id,0,0);
    SendMessage(c,WM_SETFONT,(WPARAM)f_sm,TRUE); g_ctrls.push_back(c);
}
void mkField(const std::string &lbl,const std::string &val,int x,int y,int id){
    auto l=CreateWindowW(L"STATIC",W(lbl).c_str(),WS_CHILD|WS_VISIBLE,
        x,y,110,16,g_main,0,0,0);
    SendMessage(l,WM_SETFONT,(WPARAM)f_xs,TRUE); g_ctrls.push_back(l);
    auto c=CreateWindowW(L"EDIT",W(val).c_str(),
        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,
        x,y+18,105,24,g_main,(HMENU)(INT_PTR)id,0,0);
    SendMessage(c,WM_SETFONT,(WPARAM)f_mono,TRUE); g_ctrls.push_back(c);
    g_fields.push_back({id,c});
}
void mkLabel(const std::string &s,int x,int y,int w){
    auto c=CreateWindowW(L"STATIC",W(s).c_str(),WS_CHILD|WS_VISIBLE,
        x,y,w,18,g_main,0,0,0);
    SendMessage(c,WM_SETFONT,(WPARAM)f_xs,TRUE); g_ctrls.push_back(c);
}
std::string getField(int id){
    for(auto[k,c]:g_fields) if(k==id){wchar_t b[256];GetWindowTextW(c,b,256);return N(b);}
    throw std::runtime_error("Missing field.");
}
double getNum(int id){
    auto s=getField(id);size_t u=0;double v;
    try{v=std::stod(s,&u);}catch(...){throw std::runtime_error("Enter a number.");}
    if(u!=s.size()||!std::isfinite(v)) throw std::runtime_error("Enter finite numeric values.");
    return v;
}
int getInt(int id){double v=getNum(id);if(v!=std::floor(v)||v<0||v>200000)throw std::runtime_error("Enter a whole number in range.");return(int)v;}
void beginTask(std::function<void()> work){
    if(g_busy) throw std::runtime_error("Another task is running.");
    g_busy=true;g_cancel=false;g_progress=0;g_completion={};g_error.clear();
    setStatus("Computing\u2026");
    g_job=std::thread([work]{try{work();}catch(const std::exception&e){g_error=e.what();}PostMessage(g_main,WM_APP+1,0,0);});
}
std::filesystem::path dlg(bool save,const wchar_t*flt,const wchar_t*ext){
    wchar_t nm[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=g_main;
    o.lpstrFilter=flt;o.lpstrFile=nm;o.nMaxFile=32768;o.lpstrDefExt=ext;
    o.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    if(save?GetSaveFileNameW(&o):GetOpenFileNameW(&o)) return nm; return {};
}
void resetView(){g_yaw=.6;g_pitch=.35;g_zoom=1;g_px=g_py=0;redraw();}
// ── Projection ────────────────────────────────────────────────────────────────
struct Proj{double cx=0,cy=0,cz=0,sc=1;int w,h;
    POINT pt(Vec3 p)const{
        double x=p.x-cx,y=p.y-cy,z=p.z-cz;
        double xx=cos(g_yaw)*x+sin(g_yaw)*z,zz=-sin(g_yaw)*x+cos(g_yaw)*z;
        double yy=cos(g_pitch)*y-sin(g_pitch)*zz;
        return{(LONG)(w/2+g_px+xx*sc*g_zoom),(LONG)(h/2+g_py-yy*sc*g_zoom)};
    }
};
Proj mkProj(int w,int h){
    Proj p;p.w=w;p.h=h;std::vector<Vec3>vs;
    if(g_cloud)for(auto&n:g_nodes)vs.push_back(n.p);
    else for(auto&t:g_tris)for(auto&v:t.v)vs.push_back(v);
    if(vs.empty())return p;
    Vec3 lo=vs[0],hi=vs[0];
    for(auto&v:vs){lo.x=std::min(lo.x,v.x);lo.y=std::min(lo.y,v.y);lo.z=std::min(lo.z,v.z);
        hi.x=std::max(hi.x,v.x);hi.y=std::max(hi.y,v.y);hi.z=std::max(hi.z,v.z);}
    p.cx=(lo.x+hi.x)/2;p.cy=(lo.y+hi.y)/2;p.cz=(lo.z+hi.z)/2;
    double span=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z,1e-12});
    p.sc=.6*std::min(w,h)/span; return p;
}
COLORREF heat(double t){t=std::clamp(t,0.0,1.0);
    if(t<.25)return RGB((int)(0),(int)(t*4*200),(int)(180+t*4*75));
    if(t<.5){double s=(t-.25)*4;return RGB((int)(s*85),(int)(200-s*60),(int)(255-s*120));}
    if(t<.75){double s=(t-.5)*4;return RGB((int)(85+s*170),(int)(140+s*60),(int)(135-s*100));}
    double s=(t-.75)*4;return RGB((int)(255),(int)(200-s*120),(int)(35-s*35));}
// ── Section header helper ─────────────────────────────────────────────────────
void sectionHeader(HDC dc,int x,int y,const std::string &s){
    SelectObject(dc,f_xs); drawText(dc,x,y,s,C_TXT3,f_xs);
    drawLine(dc,x,y+16,x+300,y+16,C_BORDER);
}
// ── Canvas paint ──────────────────────────────────────────────────────────────
void paint(HDC dc,int w,int h){
    fillRect(dc,{0,0,w,h},C_BG2);
    SelectObject(dc,f_ui);
    const char *titles[]={"GEOMETRY","THERMAL ANALYSIS","PERFORMANCE","HARDWARE","BACKPROPAGATION"};
    // Module title bar inside canvas
    fillRect(dc,{0,0,w,36},C_PANEL);
    drawLine(dc,0,36,w,36,C_BORDER);
    drawText(dc,12,9,titles[g_view],C_TXT2,f_xs);
    // Status badge
    if(g_busy){fillRound(dc,{w-120,8,w-10,28},RGB(37,60,50),3);drawText(dc,w-112,10,"Computing\u2026",C_GREEN,f_xs);}
    int top=44;
    if(g_view==0){
        auto p=mkProj(w,h-top);
        if(!g_cloud){
            for(const auto&t:g_tris){POINT pts[4];
                for(int i=0;i<3;++i)pts[i]={p.pt(t.v[i]).x,p.pt(t.v[i]).y+top};
                pts[3]=pts[0];
                HPEN pen=CreatePen(PS_SOLID,1,C_CYAN);auto o=SelectObject(dc,pen);
                Polyline(dc,pts,4);SelectObject(dc,o);DeleteObject(pen);}
            // Info overlay
            fillRound(dc,{8,top+8,260,top+50},C_PANEL,4);
            drawText(dc,16,top+14,"Triangles: "+std::to_string(g_tris.size()),C_TXT2,f_sm);
            drawText(dc,16,top+30,"Mode: Wireframe  |  Units: m",C_TXT3,f_xs);
        } else {
            double lo=0,hi=1;
            if(!g_nodes.empty()){lo=hi=g_nodes[0].temperature;for(auto&n:g_nodes){lo=std::min(lo,n.temperature);hi=std::max(hi,n.temperature);}}
            for(auto&n:g_nodes){auto v=p.pt(n.p);int vx=v.x,vy=v.y+top;
                fillRect(dc,{vx-3,vy-3,vx+4,vy+4},heat(hi==lo?.5:(n.temperature-lo)/(hi-lo)));}
            fillRound(dc,{8,top+8,260,top+50},C_PANEL,4);
            drawText(dc,16,top+14,"Nodes: "+std::to_string(g_nodes.size()),C_TXT2,f_sm);
            drawText(dc,16,top+30,"Mode: Point Cloud  |  Click to inspect",C_TXT3,f_xs);
            // Legend
            int lx=w-40,ly=top+20,lh=180;
            for(int i=0;i<lh;++i)fillRect(dc,{lx,ly+i,lx+16,ly+i+1},heat(1.0-i/(double)(lh-1)));
            borderRect(dc,{lx,ly,lx+16,ly+lh},C_BORDER);
            drawText(dc,lx-50,ly-14,F(hi,1)+"\u00b0C",C_TXT3,f_xs);
            drawText(dc,lx-50,ly+lh+2,F(lo,1)+"\u00b0C",C_TXT3,f_xs);
        }
        if((g_cloud&&g_nodes.empty())||(!g_cloud&&g_tris.empty()))
            drawText(dc,w/2-160,h/2,"Load a file or use a bundled sample to begin.",C_TXT3,f_sm);
        // Controls hint
        fillRound(dc,{8,h-28,480,h-6},C_PANEL,3);
        drawText(dc,16,h-24,"Drag: rotate   Right-drag: pan   Scroll: zoom   Click: inspect node",C_TXT3,f_xs);
    }
    if(g_view==1){
        if(g_sol.t.empty()){drawText(dc,20,top+20,"Configure parameters and click Solve.",C_TXT3,f_sm);return;}
        double lo=*std::min_element(g_sol.t.begin(),g_sol.t.end());
        double hi=*std::max_element(g_sol.t.begin(),g_sol.t.end());
        std::vector<std::pair<double,double>>fe,ex;
        for(size_t i=0;i<g_sol.x.size();++i)fe.push_back({g_sol.x[i],g_sol.t[i]});
        for(int i=0;i<=500;++i){double x=g_rod.L*i/500,t=exact(g_rod,x);lo=std::min(lo,t);hi=std::max(hi,t);ex.push_back({x,t});}
        double pad=std::max(1.0,(hi-lo)*.12);
        RECT r{80,top+16,w-24,h-50};
        fillRect(dc,{r.left-2,r.top-2,r.right+2,r.bottom+2},C_PANEL);
        // Grid
        for(int i=0;i<=5;++i){
            int y=r.bottom-(r.bottom-r.top)*i/5;drawLine(dc,r.left,y,r.right,y,C_GRID);
            drawText(dc,4,y-7,F(lo-pad+(hi-lo+2*pad)*i/5,3),C_TXT3,f_xs);
            int x=r.left+(r.right-r.left)*i/5;
            drawText(dc,x-10,r.bottom+4,F(g_rod.L*i/5.0,3),C_TXT3,f_xs);
        }
        drawLine(dc,r.left,r.bottom,r.right,r.bottom,C_TXT3,2);
        drawLine(dc,r.left,r.top,r.left,r.bottom,C_TXT3,2);
        // Plot curves
        auto plotPts=[&](const std::vector<std::pair<double,double>>&pts,COLORREF col,int pw){
            HPEN pen=CreatePen(PS_SOLID,pw,col);auto o=SelectObject(dc,pen);bool first=true;
            for(auto&[x,y]:pts){int px=r.left+(int)((x/g_rod.L)*(r.right-r.left));
                int py=r.bottom-(int)(((y-(lo-pad))/(hi-lo+2*pad))*(r.bottom-r.top));
                if(first)MoveToEx(dc,px,py,0);else LineTo(dc,px,py);first=false;}
            SelectObject(dc,o);DeleteObject(pen);};
        plotPts(ex,C_WARN,2); plotPts(fe,C_CYAN,2);
        // Legend
        fillRound(dc,{r.left+8,r.top+6,r.left+300,r.top+24},RGB(20,22,28),3);
        fillRect(dc,{r.left+14,r.top+12,r.left+30,r.top+18},C_CYAN);
        drawText(dc,r.left+34,r.top+8,"FE (N="+std::to_string(g_rod.n)+")",C_TXT2,f_xs);
        fillRect(dc,{r.left+150,r.top+12,r.left+166,r.top+18},C_WARN);
        drawText(dc,r.left+170,r.top+8,"Analytical exact",C_TXT2,f_xs);
        // Results panel
        fillRound(dc,{r.left,h-46,w-24,h-8},C_PANEL,3);
        drawText(dc,r.left+8,h-42,"Max midpoint error: "+F(g_sol.midpoint_error,10)+" \u00b0C",C_TXT,f_sm);
        drawText(dc,r.left+360,h-42,"x (m)",C_TXT3,f_xs);
        drawText(dc,4,top+4,"T(\u00b0C)",C_TXT3,f_xs);
    }
    if(g_view==2){
        sectionHeader(dc,16,top,"BENCHMARK RESULTS");
        if(g_timings.empty()){drawText(dc,16,top+28,"Run benchmark to see results.",C_TXT3,f_sm);return;}
        // Table header
        int y=top+28; const int cols[]={16,100,220,340,440,580};
        drawText(dc,cols[0],y,"Workers",C_TXT3,f_xs);drawText(dc,cols[1],y,"Median (s)",C_TXT3,f_xs);
        drawText(dc,cols[2],y,"Speedup",C_TXT3,f_xs);drawText(dc,cols[3],y,"Efficiency",C_TXT3,f_xs);
        drawText(dc,cols[4],y,"Max \u0394T (\u00b0C)",C_TXT3,f_xs);y+=20;
        drawLine(dc,16,y,w-16,y,C_BORDER);y+=6;
        double maxM=0;for(auto&r:g_timings)maxM=std::max(maxM,r.median);
        for(auto&r:g_timings){
            fillRound(dc,{14,y-2,w-14,y+22},C_PANEL,3);
            drawText(dc,cols[0],y+2,std::to_string(r.workers),C_TXT,f_sm);
            drawText(dc,cols[1],y+2,F(r.median,6),C_TXT,f_mono);
            drawText(dc,cols[2],y+2,F(r.speedup,3)+"x",C_CYAN,f_mono);
            drawText(dc,cols[3],y+2,F(100*r.efficiency,1)+"%",r.efficiency>.8?C_GREEN:C_WARN,f_mono);
            drawText(dc,cols[4],y+2,F(r.max_difference,3),r.max_difference<1e-6?C_GREEN:C_ERR,f_mono);
            y+=28;
        }
        y+=12;sectionHeader(dc,16,y,"EXECUTION TIME");y+=24;
        for(auto&r:g_timings){int bw=(int)((w-200)*r.median/std::max(maxM,1e-12));
            fillRound(dc,{100,y,100+bw,y+18},C_CYAN,3);
            drawText(dc,16,y+1,std::to_string(r.workers)+"w",C_TXT2,f_xs);
            drawText(dc,106+bw,y+1,F(r.median,4)+"s",C_TXT3,f_xs);y+=26;}
    }
    if(g_view==3){
        auto&d=g_devs[g_devi];
        sectionHeader(dc,16,top,"DEVICE STATUS");
        fillRound(dc,{14,top+24,w-14,top+72},C_PANEL,4);
        drawText(dc,24,top+28,d.name,C_TXT,f_ui);
        COLORREF sc=d.connected?C_GREEN:C_ERR;
        fillRound(dc,{24,top+48,14+ (d.connected?110:130),top+64},d.connected?RGB(30,60,45):RGB(60,30,30),3);
        drawText(dc,32,top+49,d.connected?"CONNECTED":"DISCONNECTED",sc,f_xs);
        drawText(dc,24,top+80,d.capabilities,C_TXT3,f_sm);
        int y=top+108;sectionHeader(dc,16,y,"SYSTEM TOPOLOGY");y+=22;
        const char*rows[]={"Host C++ adapter \u2500\u2500 USB \u2500\u25b6  NUCLEO-F401RE   MCU telemetry, 3.3V GPIO",
            "Host C++ adapter \u2500\u2500 USB \u2500\u25b6  Basys 3         USB-UART, JTAG programming",
            "Host C++ adapter \u2500\u2500 USB \u2500\u25b6  J-Link probe    SWD debug, VTref sense"};
        for(auto s:rows){fillRound(dc,{14,y,w-14,y+22},C_PANEL,3);drawText(dc,24,y+3,s,C_TXT2,f_xs);y+=28;}
        y+=8;drawText(dc,16,y,"All connections are SIMULATED. No physical hardware required.",C_TXT3,f_xs);
    }
    if(g_view==4){
        Network p;
        std::vector<std::pair<double,double>>data;
        for(int i=0;i<=101;++i){data.push_back({(double)i,loss(p)});if(i<101)p=update(p);}
        sectionHeader(dc,16,top,"MODEL");
        drawText(dc,16,top+22,"h = tanh(wx+b)    \u0177 = vh+c    J = \u00bd(\u0177\u2212y)\u00b2    x=1, y=2, \u03b7=0.1",C_TXT2,f_sm);
        RECT r{80,top+50,w-24,h-40};
        fillRect(dc,{r.left-2,r.top-2,r.right+2,r.bottom+2},C_PANEL);
        // Grid
        for(int i=0;i<=5;++i){int y=r.bottom-(r.bottom-r.top)*i/5;drawLine(dc,r.left,y,r.right,y,C_GRID);
            drawText(dc,10,y-7,F(1.25*i/5.0,2),C_TXT3,f_xs);
            int x=r.left+(r.right-r.left)*i/5;drawText(dc,x-6,r.bottom+4,std::to_string(i*101/5),C_TXT3,f_xs);}
        drawLine(dc,r.left,r.bottom,r.right,r.bottom,C_TXT3,2);
        drawLine(dc,r.left,r.top,r.left,r.bottom,C_TXT3,2);
        // Curve
        HPEN pen=CreatePen(PS_SOLID,2,C_CYAN);auto o=SelectObject(dc,pen);bool first=true;
        for(auto&[x,y]:data){int px=r.left+(int)(x/101*(r.right-r.left));
            int py=r.bottom-(int)(y/1.25*(r.bottom-r.top));
            if(first)MoveToEx(dc,px,py,0);else LineTo(dc,px,py);first=false;}
        SelectObject(dc,o);DeleteObject(pen);
        drawText(dc,r.left+8,r.top+6,"Loss J vs Update",C_TXT2,f_xs);
        drawText(dc,16,h-36,"Initial: "+F(loss({}),8)+"   Final: "+F(loss(p),8),C_TXT3,f_sm);
    }
}
// ── File operations ───────────────────────────────────────────────────────────
void loadFile(std::filesystem::path path,bool csv){
    beginTask([path,csv]{auto content=read_text(path);
        if(csv){auto parsed=parse_csv(content);g_completion=[parsed=std::move(parsed)]{g_nodes=parsed;g_cloud=true;resetView();setStatus("CSV imported: "+std::to_string(g_nodes.size())+" nodes");};}
        else{auto parsed=parse_stl(content);g_completion=[parsed=std::move(parsed)]{g_tris=parsed;g_cloud=false;resetView();setStatus("STL imported: "+std::to_string(g_tris.size())+" triangles");};}
    });
}
void exportCSV(const std::string &s){auto p=dlg(true,L"CSV\0*.csv\0\0",L"csv");if(!p.empty()){write_text(p,s);setStatus("Exported.");}}
// ── Command dispatch ──────────────────────────────────────────────────────────
void layout();
void command(int id){
    if(id>=100&&id<=104){g_view=id-100;layout();return;}
    if(id==190){g_cancel=true;setStatus("Cancelling\u2026");return;}
    if(id==200||id==201){auto p=dlg(false,id==200?L"STL\0*.stl\0\0":L"CSV\0*.csv\0\0",0);if(!p.empty())loadFile(p,id==201);}
    if(id==202||id==203)loadFile(g_root/"data"/(id==202?"rod.stl":"synthetic_temperature.csv"),id==203);
    if(id==204)resetView();
    if(id==205){g_cloud=!g_cloud;layout();}
    if(id==206){
        if(g_nodes.empty()){
            setStatus("No nodes to export – load a CSV first.");
            return;
        }
        std::ostringstream s;
        s<<std::setprecision(17)<<"node_id,x_m,y_m,z_m,temperature_C\n";
        for(auto& n:g_nodes) s<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<','<<n.temperature<<'\n';
        exportCSV(s.str());
    }
        validate(r);beginTask([r]{auto s=solve(r);g_completion=[r,s=std::move(s)]{g_rod=r;g_sol=s;setStatus("Solved. Error: "+F(s.midpoint_error,10)+"\u00b0C");g_view=1;layout();redraw();};});}
    if(id==311){g_rod={};g_sol={};layout();}
    if(id==312){if(g_sol.t.empty())throw std::runtime_error("Solve first.");exportCSV(thermal_csv(g_rod,g_sol));}
    if(id==313){std::ostringstream s;s<<std::setprecision(17)<<"elements,h_m,max_midpoint_error_C\n";std::string msg;
        for(int n:{10,20,40,80}){Rod r;r.n=n;auto sol=solve(r);s<<n<<','<<1.0/n<<','<<sol.midpoint_error<<'\n';
            msg+="N="+std::to_string(n)+": "+F(sol.midpoint_error,10)+"\u00b0C\n";}
        MessageBoxW(g_main,W(msg).c_str(),L"Convergence",MB_OK);exportCSV(s.str());}
    if(id==410){int c=getInt(400),m=getInt(401);generate_cases(c,m);
        beginTask([c,m]{auto rows=benchmark(c,m,&g_cancel,&g_progress);
            g_completion=[rows=std::move(rows),c,m]{g_timings=rows;g_bcnt=c;g_bmesh=m;setStatus("Benchmark complete.");};});}
    if(id==411){if(g_timings.empty())throw std::runtime_error("Run benchmark first.");exportCSV(timing_csv(g_timings,g_bcnt,g_bmesh));}
    if(id==412)MessageBoxW(g_main,W(system_info()).c_str(),L"System Info",MB_OK);
    if(id>=500&&id<=502){g_devi=id-500;redraw();}
    auto&d=g_devs[g_devi];
    if(id==510){d.connect();setStatus(d.name+" connected (SIM).");redraw();}
    if(id==511){d.disconnect();setStatus("Disconnected.");redraw();}
    if(id==512)setStatus(d.telemetry());
    if(id==513){d.fail_next=true;setStatus("Next read will timeout; retry recovers.");}
    if(id==600){auto g=gradient({}),n=numerical_gradient({});std::string msg;const char*pn[]={"w","b","v","c"};
        for(int i=0;i<4;++i)msg+=std::string(pn[i])+": analytic="+F(g[i],10)+" fd="+F(n[i],10)+" err="+F(std::abs(g[i]-n[i]),4)+"\n";
        MessageBoxW(g_main,W(msg).c_str(),L"Gradient Check",MB_OK);}
    if(id==601)exportCSV(training_csv());
    redraw();
}
// ── Layout - builds controls for current view ─────────────────────────────────
void layout(){
    for(auto c:g_ctrls)DestroyWindow(c);g_ctrls.clear();g_fields.clear();
    RECT cr;GetClientRect(g_main,&cr);
    int cx=SIDEBAR_W+8, cy=TOPBAR_H+8, cw=cr.right-SIDEBAR_W-16;
    if(g_view==0){
        mkBtn("Open STL",cx,cy,100,200);mkBtn("Open CSV",cx+108,cy,100,201);
        mkBtn("STL sample",cx+216,cy,100,202);mkBtn("CSV sample",cx+324,cy,100,203);
        mkBtn("Fit",cx+432,cy,60,204);mkBtn(g_cloud?"Mesh":"Nodes",cx+500,cy,70,205);
        mkBtn("Export",cx+578,cy,80,206);
    }
    if(g_view==1){
        mkField("Elements",std::to_string(g_rod.n),cx,cy,300);
        mkField("Length (m)",F(g_rod.L),cx+118,cy,301);
        mkField("Area (m\u00b2)",F(g_rod.A),cx+236,cy,302);
        mkField("k (W/mK)",F(g_rod.k),cx+354,cy,303);
        mkField("q (W/m\u00b3)",F(g_rod.q),cx+472,cy,304);
        mkField("T\u2080 (\u00b0C)",F(g_rod.left),cx+590,cy,305);
        mkField("T\u2097 (\u00b0C)",F(g_rod.right),cx+708,cy,306);
        mkBtn("Solve",cx,cy+50,90,310,true);mkBtn("Defaults",cx+98,cy+50,90,311);
        mkBtn("Export CSV",cx+196,cy+50,110,312);mkBtn("Convergence",cx+314,cy+50,120,313);
    }
    if(g_view==2){
        mkField("Cases",std::to_string(g_bcnt),cx,cy,400);
        mkField("Elements",std::to_string(g_bmesh),cx+118,cy,401);
        mkBtn("Run benchmark",cx+240,cy+18,140,410,true);mkBtn("Export",cx+388,cy+18,90,411);
        mkBtn("Machine info",cx+486,cy+18,110,412);
    }
    if(g_view==3){
        for(int i=0;i<3;++i)mkBtn(g_devs[i].name,cx+i*220,cy,210,500+i);
        mkBtn("Connect",cx,cy+38,110,510,true);mkBtn("Disconnect",cx+118,cy+38,110,511);
        mkBtn("Read telemetry",cx+236,cy+38,130,512);mkBtn("Inject timeout",cx+374,cy+38,130,513);
    }
    if(g_view==4){
        mkBtn("Gradient check",cx,cy,150,600,true);mkBtn("Export 101 updates",cx+158,cy,170,601);
    }
    // Canvas and status
    int canvasTop=TOPBAR_H+(g_view==1?100:80);
    MoveWindow(g_canvas,SIDEBAR_W,canvasTop,cr.right-SIDEBAR_W,cr.bottom-canvasTop-32,TRUE);
    MoveWindow(g_status,SIDEBAR_W,cr.bottom-32,cr.right-SIDEBAR_W,32,TRUE);
    redraw();
}
// ── Canvas proc ───────────────────────────────────────────────────────────────
LRESULT CALLBACK CanvasProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hw,&ps);RECT r;GetClientRect(hw,&r);
        HDC mem=CreateCompatibleDC(dc);HBITMAP bmp=CreateCompatibleBitmap(dc,r.right,r.bottom);
        auto o=SelectObject(mem,bmp);paint(mem,r.right,r.bottom);
        BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);
        SelectObject(mem,o);DeleteObject(bmp);DeleteDC(mem);EndPaint(hw,&ps);return 0;}
    case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:
        g_drag=true;g_pan=(msg==WM_RBUTTONDOWN);g_prev={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};SetCapture(hw);return 0;
    case WM_MOUSEMOVE:if(g_drag&&g_view==0){int dx=GET_X_LPARAM(lp)-g_prev.x,dy=GET_Y_LPARAM(lp)-g_prev.y;
        if(g_pan){g_px+=dx;g_py+=dy;}else{g_yaw+=dx*.01;g_pitch+=dy*.01;}g_prev={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};redraw();}return 0;
    case WM_LBUTTONUP:case WM_RBUTTONUP:g_drag=false;ReleaseCapture();
        if(msg==WM_LBUTTONUP&&g_view==0&&g_cloud&&!g_nodes.empty()){
            RECT r;GetClientRect(hw,&r);auto p=mkProj(r.right,r.bottom);
            double best=100;const Node*sel=0;
            for(const auto&n:g_nodes){auto v=p.pt(n.p);double dx=v.x-GET_X_LPARAM(lp),dy=v.y-GET_Y_LPARAM(lp);
                double d=dx*dx+dy*dy;if(d<best){best=d;sel=&n;}}
            if(sel)setStatus("Node "+std::to_string(sel->id)+" | "+F(sel->p.x)+"m, "+F(sel->p.y)+"m, "+F(sel->p.z)+"m | T="+F(sel->temperature)+"\u00b0C");}return 0;
    case WM_MOUSEWHEEL:if(g_view==0){g_zoom=std::clamp(g_zoom*(GET_WHEEL_DELTA_WPARAM(wp)>0?1.12:1/1.12),.1,100.0);redraw();}return 0;
    }return DefWindowProcW(hw,msg,wp,lp);
}
// ── Owner-draw buttons ────────────────────────────────────────────────────────
void paintBtn(DRAWITEMSTRUCT*di){
    HDC dc=di->hDC;RECT r=di->rcItem;int id=(int)(INT_PTR)GetMenu(di->hwndItem);
    bool pressed=(di->itemState&ODS_SELECTED)!=0;
    bool is_primary=(id==310||id==410||id==510||id==600);
    COLORREF bg=pressed?C_BTN_H:C_BTN, fg=C_TXT;
    if(is_primary){bg=pressed?RGB(60,170,200):C_CYAN;fg=C_BG;}
    if(id==190){bg=pressed?RGB(180,60,60):RGB(80,30,30);fg=C_ERR;}
    fillRound(dc,r,bg,4);
    wchar_t txt[256]{};GetWindowTextW(di->hwndItem,txt,256);
    SetTextColor(dc,fg);SetBkMode(dc,TRANSPARENT);SelectObject(dc,f_sm);
    DrawTextW(dc,txt,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
// ── Sidebar drawing ───────────────────────────────────────────────────────────
void drawSidebar(HDC dc,int h){
    fillRect(dc,{0,0,SIDEBAR_W,h},C_SIDEBAR);
    drawLine(dc,SIDEBAR_W-1,0,SIDEBAR_W-1,h,C_BORDER);
    // Logo area
    SelectObject(dc,f_title);drawText(dc,16,16,"Thermo",C_CYAN,f_title);
    drawText(dc,16+7*12,16,"Bridge",C_TXT,f_title);
    SelectObject(dc,f_xs);drawText(dc,16,42,"Engineering Workbench",C_TXT3,f_xs);
    drawLine(dc,16,62,SIDEBAR_W-16,62,C_BORDER);
    // Nav items
    const char*items[]={"Geometry","Thermal Analysis","Performance","Hardware","Backpropagation"};
    const char*icons[]={"\u25a2","\u2261","\u25b6","\u2302","\u2207"};
    for(int i=0;i<5;++i){
        int y=72+i*36;RECT r={0,y,SIDEBAR_W-1,y+34};
        if(i==g_view){fillRect(dc,r,C_SIDEBAR_A);fillRect(dc,{0,y,3,y+34},C_CYAN);}
        drawText(dc,16,y+9,icons[i],i==g_view?C_CYAN:C_TXT3,f_sm);
        drawText(dc,34,y+9,items[i],i==g_view?C_TXT:C_TXT2,f_sm);
    }
    // Bottom
    drawLine(dc,16,h-40,SIDEBAR_W-16,h-40,C_BORDER);
    drawText(dc,16,h-30,"v1.0  \u00b7  C++17",C_TXT3,f_xs);
}
// ── Main window proc ──────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:{g_main=hw;
        f_title=CreateFontW(-22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        f_ui=CreateFontW(-15,0,0,0,FW_MEDIUM,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        f_sm=CreateFontW(-13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        f_xs=CreateFontW(-11,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        f_mono=CreateFontW(-13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Consolas");
        br_bg=CreateSolidBrush(C_BG);br_panel=CreateSolidBrush(C_PANEL);br_sidebar=CreateSolidBrush(C_SIDEBAR);
        g_canvas=CreateWindowW(L"TBCanvas",L"",WS_CHILD|WS_VISIBLE,0,0,100,100,hw,0,0,0);
        g_status=CreateWindowW(L"STATIC",W(g_notice).c_str(),WS_CHILD|WS_VISIBLE|SS_LEFT,0,0,100,30,hw,0,0,0);
        SendMessage(g_status,WM_SETFONT,(WPARAM)f_sm,TRUE);
        layout();SetTimer(hw,1,200,0);return 0;}
    case WM_GETMINMAXINFO:{auto*p=(MINMAXINFO*)lp;p->ptMinTrackSize={1100,750};return 0;}
    case WM_SIZE:if(g_canvas)layout();return 0;
    case WM_CTLCOLORSTATIC:{HDC dc=(HDC)wp;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,C_TXT2);return(LRESULT)br_bg;}
    case WM_DRAWITEM:paintBtn((DRAWITEMSTRUCT*)lp);return TRUE;
    case WM_COMMAND:try{command(LOWORD(wp));}catch(const std::exception&e){setStatus(e.what());MessageBoxW(hw,W(e.what()).c_str(),L"Error",MB_OK|MB_ICONWARNING);}return 0;
    case WM_APP+1:if(g_job.joinable())g_job.join();g_busy=false;
        if(!g_error.empty())setStatus(g_error);else if(g_cancel)setStatus("Cancelled.");else if(g_completion)g_completion();
        g_completion={};redraw();return 0;
    case WM_TIMER:if(g_busy&&g_view==2)setStatus("Benchmark: "+std::to_string(g_progress.load())+"%");
        if(!g_ready){g_ready=true;double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-g_start).count();
            try{std::filesystem::create_directories(g_root/"results");std::ostringstream o;
                o<<"entry_to_first_ready_timer_seconds="<<el<<"\n";
                write_text(g_root/"results"/("runtime_ready_"+std::to_string(GetCurrentProcessId())+".txt"),o.str());}catch(...){}}return 0;
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hw,&ps);RECT r;GetClientRect(hw,&r);
        // Background
        fillRect(dc,{0,0,r.right,r.bottom},C_BG);
        // Sidebar
        drawSidebar(dc,r.bottom);
        // Top bar
        fillRect(dc,{SIDEBAR_W,0,r.right,TOPBAR_H},C_BG);
        drawLine(dc,SIDEBAR_W,TOPBAR_H-1,r.right,TOPBAR_H-1,C_BORDER);
        const char*ttl[]={"Geometry","Thermal Analysis","Performance","Hardware","Backpropagation"};
        drawText(dc,SIDEBAR_W+16,14,ttl[g_view],C_TXT,f_ui);
        if(g_busy){fillRound(dc,{r.right-150,12,r.right-16,34},RGB(30,50,45),3);drawText(dc,r.right-142,15,"Computing\u2026",C_GREEN,f_xs);}
        // Cancel button in topbar
        // Status bar bg
        fillRect(dc,{SIDEBAR_W,r.bottom-32,r.right,r.bottom},C_PANEL);
        drawLine(dc,SIDEBAR_W,r.bottom-32,r.right,r.bottom-32,C_BORDER);
        drawText(dc,SIDEBAR_W+12,r.bottom-26,g_notice,C_TXT2,f_xs);
        // Sidebar nav click zones (handled in WM_LBUTTONDOWN)
        EndPaint(hw,&ps);return 0;}
    case WM_LBUTTONDOWN:{int mx=GET_X_LPARAM(lp),my=GET_Y_LPARAM(lp);
        if(mx<SIDEBAR_W&&my>=72&&my<72+5*36){int idx=(my-72)/36;if(idx>=0&&idx<5){g_view=idx;layout();InvalidateRect(hw,0,TRUE);}}return 0;}
    case WM_CLOSE:if(g_busy){g_cancel=true;if(g_job.joinable())g_job.join();}DestroyWindow(hw);return 0;
    case WM_DESTROY:DeleteObject(f_title);DeleteObject(f_ui);DeleteObject(f_sm);DeleteObject(f_xs);DeleteObject(f_mono);
        DeleteObject(br_bg);DeleteObject(br_panel);DeleteObject(br_sidebar);PostQuitMessage(0);return 0;
    }return DefWindowProcW(hw,msg,wp,lp);
}
} // namespace
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int show){
    SetProcessDPIAware();
    wchar_t path[32768];GetModuleFileNameW(0,path,32768);
    g_root=std::filesystem::path(path).parent_path();
    WNDCLASSW wc{};wc.lpfnWndProc=CanvasProc;wc.hInstance=inst;wc.lpszClassName=L"TBCanvas";wc.hCursor=LoadCursor(0,IDC_ARROW);RegisterClassW(&wc);
    wc.lpfnWndProc=WndProc;wc.lpszClassName=L"ThermoBridge";wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
    HWND w=CreateWindowW(L"ThermoBridge",L"ThermoBridge | Engineering Workbench",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1200,850,0,0,inst,0);
    if(!w)return 1;ShowWindow(w,show);UpdateWindow(w);
    MSG m{};while(GetMessageW(&m,0,0,0)>0){TranslateMessage(&m);DispatchMessageW(&m);}return(int)m.wParam;
}
