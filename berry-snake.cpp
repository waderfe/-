// Berry Snake: a standalone Windows C++ / GDI+ cartoon game.
// Build: g++ berry-snake.cpp -std=c++17 -O2 -municode -mwindows -static -o berry-snake.exe -lgdiplus -lgdi32 -luser32 -ladvapi32
// Portable logic verification: g++ berry-snake.cpp -std=c++17 -DBERRY_SNAKE_SELF_TEST -o snake-tests.exe
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct Cell {
    int x, y;
    bool operator==(const Cell& other) const { return x == other.x && y == other.y; }
};
enum class State { Ready, Running, Paused, Lost, Won };
class SnakeGame {
public:
    static constexpr int BoardSize = 20;
    std::vector<Cell> snake;
    Cell direction{1, 0}, food{14, 8};
    std::deque<Cell> turns;
    State state = State::Ready;
    int score = 0, best = 0, level = 1, difficulty = 1;
    std::mt19937 random{std::random_device{}()};

    SnakeGame() { resetBoard(); }
    void resetBoard() {
        snake = {{10, 10}, {9, 10}, {8, 10}, {7, 10}};
        direction = {1, 0}; food = {14, 8}; turns.clear(); score = 0; level = 1;
    }
    void start(int selectedDifficulty) {
        resetBoard(); difficulty = selectedDifficulty; state = State::Running;
    }
    int interval() const {
        static constexpr int rates[] = {225, 165, 115};
        return std::max(70, rates[difficulty] - (level - 1) * 12);
    }
    void turn(Cell next) {
        if (state != State::Running || turns.size() >= 2) return;
        const Cell last = turns.empty() ? direction : turns.back();
        if (next == last || (next.x == -last.x && next.y == -last.y)) return;
        turns.push_back(next);
    }
    void pause() { if (state == State::Running) state = State::Paused; }
    void resume() { if (state == State::Paused) state = State::Running; }
    bool spawnFood() {
        std::array<bool, BoardSize * BoardSize> occupied{};
        for (const auto& p : snake) occupied[p.y * BoardSize + p.x] = true;
        std::vector<Cell> empty;
        for (int y = 0; y < BoardSize; ++y)
            for (int x = 0; x < BoardSize; ++x)
                if (!occupied[y * BoardSize + x]) empty.push_back({x, y});
        if (empty.empty()) { state = State::Won; return false; }
        std::uniform_int_distribution<std::size_t> distribution(0, empty.size() - 1);
        food = empty[distribution(random)]; return true;
    }
    bool step() {
        if (state != State::Running) return false;
        if (!turns.empty()) { direction = turns.front(); turns.pop_front(); }
        const Cell head{snake.front().x + direction.x, snake.front().y + direction.y};
        const bool eating = head == food;
        // On a non-growing move the tail vacates its square before collision.
        const std::size_t occupiedCount = snake.size() - (eating ? 0 : 1);
        if (head.x < 0 || head.y < 0 || head.x >= BoardSize || head.y >= BoardSize ||
            std::find(snake.begin(), snake.begin() + occupiedCount, head) != snake.begin() + occupiedCount) {
            state = State::Lost; return false;
        }
        snake.insert(snake.begin(), head);
        if (eating) {
            score += 10; best = std::max(best, score); level = 1 + score / 50; spawnFood();
        } else snake.pop_back();
        return eating;
    }
};

#ifdef BERRY_SNAKE_SELF_TEST
int main() {
    SnakeGame g;
    g.start(1); g.step(); assert((g.snake.front() == Cell{11, 10}));
    g.start(1); g.turn({-1, 0}); assert(g.turns.empty());
    g.turn({0, -1}); g.turn({-1, 0}); g.step(); assert((g.snake.front() == Cell{10, 9}));
    g.step(); assert((g.snake.front() == Cell{9, 9}));
    g.start(1); g.food = {11, 10}; g.score = 40;
    assert(g.step()); assert(g.score == 50 && g.level == 2 && g.snake.size() == 5);
    assert(std::find(g.snake.begin(), g.snake.end(), g.food) == g.snake.end());
    g.snake = {{19, 10}, {18, 10}}; g.direction = {1, 0}; g.food = {1, 1}; g.turns.clear();
    g.step(); assert(g.state == State::Lost);
    g.start(1); g.snake = {{2, 2}, {2, 3}, {3, 3}, {3, 2}, {4, 2}};
    g.food = {1, 1}; g.step(); assert(g.state == State::Lost);
    g.start(1); g.snake = {{2, 2}, {2, 3}, {3, 3}, {3, 2}};
    g.food = {1, 1}; g.step(); assert(g.state == State::Running && g.snake.front().x == 3);
    g.pause(); auto before = g.snake; g.step(); assert(g.snake == before);
    g.resume(); assert(g.state == State::Running);
    g.start(1); g.snake = {{0, 0}};
    for (int y = 0; y < 20; ++y) for (int x = 0; x < 20; ++x)
        if (!(y == 0 && (x == 0 || x == 1))) g.snake.push_back({x, y});
    g.food = {1, 0}; g.score = 3950; g.step();
    assert(g.state == State::Won && g.snake.size() == 400 && g.score == 3960);
    g.start(0); assert(g.interval() == 225 && g.score == 0 && g.snake.size() == 4);
    g.start(2); assert(g.interval() == 115); g.level = 100; assert(g.interval() == 70);
    std::cout << "PASS: movement, reversal prevention, queued turns, growth, food placement, levels, wall/body/tail collision, pause, victory, restart, difficulty.\n";
}
#else
#ifndef _WIN32
#error "The graphical game uses Windows GDI+. Define BERRY_SNAKE_SELF_TEST for portable game-logic tests."
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
namespace {
    constexpr float DesignW = 940, DesignH = 840;
    constexpr float BoardX = 40, BoardY = 183, BoardW = 560, Unit = BoardW / SnakeGame::BoardSize;
    SnakeGame game;
    HWND mainWindow = nullptr;
    ULONG_PTR gdiplusToken = 0;
    float screenScale = 1, offsetX = 0, offsetY = 0;
    int selectedDifficulty = 1, savedBest = 0;
    ULONGLONG lastTime = 0, frameTime = 0;
    int accumulator = 0;

    Color color(unsigned int hex, BYTE alpha = 255) {
        return Color(alpha, (hex >> 16) & 255, (hex >> 8) & 255, hex & 255);
    }
    void rounded(Graphics& g, RectF r, float radius, unsigned int fill, unsigned int outline = 0, float width = 2) {
        GraphicsPath path; float d = radius * 2;
        path.AddArc(r.X, r.Y, d, d, 180, 90);
        path.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
        path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
        path.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90); path.CloseFigure();
        SolidBrush brush(color(fill)); g.FillPath(&brush, &path);
        if (outline) { Pen pen(color(outline), width); g.DrawPath(&pen, &path); }
    }
    void ellipse(Graphics& g, float x, float y, float rx, float ry, unsigned int fill) {
        SolidBrush brush(color(fill)); g.FillEllipse(&brush, x - rx, y - ry, rx * 2, ry * 2);
    }
    void text(Graphics& g, const std::wstring& value, RectF r, float size, unsigned int fill, bool bold = false, bool center = false) {
        FontFamily family(L"Microsoft YaHei"); Font font(&family, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
        SolidBrush brush(color(fill)); StringFormat format;
        format.SetAlignment(center ? StringAlignmentCenter : StringAlignmentNear);
        format.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(value.c_str(), static_cast<INT>(value.size()), &font, r, &format, &brush);
    }
    void line(Graphics& g, PointF a, PointF b, unsigned int fill, float width) {
        Pen pen(color(fill), width); pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound); g.DrawLine(&pen, a, b);
    }
    void button(Graphics& g, RectF r, const std::wstring& label, bool primary = false, bool active = false) {
        rounded(g, RectF(r.X, r.Y + 4, r.Width, r.Height), 13, primary ? 0xD48D9B : 0xC4D5AA);
        rounded(g, r, 13, primary ? 0xF4ADBC : active ? 0xDDEDC6 : 0xF1F4E7, primary ? 0xAD727E : 0xC4D5AA);
        text(g, label, r, 15, primary ? 0x684650 : 0x66834B, true, true);
    }
    void saveRecord() {
        if (game.best <= savedBest) return;
        DWORD value = static_cast<DWORD>(game.best);
        if (RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\BerrySnake", L"BestScore", REG_DWORD, &value, sizeof(value)) == ERROR_SUCCESS)
            savedBest = game.best;
    }
    void startGame() { game.start(selectedDifficulty); accumulator = 0; lastTime = GetTickCount64(); InvalidateRect(mainWindow, nullptr, FALSE); }
    void togglePause() {
        if (game.state == State::Running) game.pause();
        else if (game.state == State::Paused) { game.resume(); accumulator = 0; lastTime = GetTickCount64(); }
    }
    void strawberry(Graphics& g, float x, float y, float size = 1) {
        auto saved = g.Save(); g.TranslateTransform(x, y); g.ScaleTransform(size, size);
        ellipse(g, 0, 10, 12, 4, 0xB5C78F);
        GraphicsPath berry;
        berry.AddBezier(0, -8, -17, -17, -18, 0, -8, 11);
        berry.AddBezier(-8, 11, -4, 18, 0, 20, 0, 20);
        berry.AddBezier(0, 20, 4, 18, 8, 11, 8, 11);
        berry.AddBezier(8, 11, 18, 0, 17, -17, 0, -8); berry.CloseFigure();
        SolidBrush pink(color(0xEF8195)); Pen outline(color(0xB86976), 1.6f); g.FillPath(&pink, &berry); g.DrawPath(&outline, &berry);
        PointF leaves[] = {{0,-7},{-11,-13},{-3,-12},{1,-19},{4,-12},{12,-13},{4,-5}};
        SolidBrush green(color(0x6C9D4C)); g.FillPolygon(&green, leaves, 7);
        for (PointF p : {PointF(-8,-3),PointF(2,-1),PointF(9,0),PointF(-4,7),PointF(5,9),PointF(0,14)})
            ellipse(g, p.X, p.Y, 1.2f, 1.8f, 0xFFF0BF);
        ellipse(g, -10, -3, 2, 4, 0xFFB2BF); g.Restore(saved);
    }
    void mascot(Graphics& g, float x, float y, float scale) {
        auto saved = g.Save(); g.TranslateTransform(x,y); g.ScaleTransform(scale,scale);
        GraphicsPath body; body.AddLine(10,65,60,65); body.AddBezier(60,65,85,65,88,55,88,28);
        Pen border(color(0x648D46),26); border.SetStartCap(LineCapRound); border.SetEndCap(LineCapRound); g.DrawPath(&border,&body);
        Pen light(color(0x9BCD6A),19); light.SetStartCap(LineCapRound); light.SetEndCap(LineCapRound); g.DrawPath(&light,&body);
        ellipse(g,88,18,21,23,0xA6D675); ellipse(g,80,11,6,8,0xFFFDF2);ellipse(g,98,11,6,8,0xFFFDF2);
        ellipse(g,82,12,2.8f,3,0x364E33);ellipse(g,100,12,2.8f,3,0x364E33);
        ellipse(g,72,24,4,2.5f,0xEFB1A9);ellipse(g,107,24,4,2.5f,0xEFB1A9);
        GraphicsPath smile; smile.AddBezier(82,28,87,33,94,33,99,27); Pen pen(color(0x52783B),2);g.DrawPath(&pen,&smile);
        g.Restore(saved);
    }
    void flower(Graphics& g,float x,float y,unsigned int fill) {
        for (int i=0;i<5;++i) ellipse(g,x+std::cos(i*1.2566f)*3,y+std::sin(i*1.2566f)*3,2.5f,2.5f,fill);
        ellipse(g,x,y,2,2,0xDDC36A);
    }
    void drawSnake(Graphics& g) {
        for (int layer=0;layer<3;++layer) {
            GraphicsPath path;
            for (std::size_t i=1;i<game.snake.size();++i) {
                const auto a=game.snake[i-1],b=game.snake[i]; const float dy=layer==0?3:layer==2?-1:0;
                path.AddLine(BoardX+(a.x+.5f)*Unit,BoardY+(a.y+.5f)*Unit+dy,BoardX+(b.x+.5f)*Unit,BoardY+(b.y+.5f)*Unit+dy);
            }
            Pen pen(color(layer==0?0x91AD74:layer==1?0x5D8841:0x9DCC65),layer==2?19.0f:24.0f);
            pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);g.DrawPath(&pen,&path);
        }
        for (std::size_t i=1;i<game.snake.size();i+=2) {
            const auto p=game.snake[i];ellipse(g,BoardX+(p.x+.5f)*Unit,BoardY+(p.y+.5f)*Unit-3,3,2,0xC4E692);
        }
        auto saved=g.Save(); const Cell head=game.snake.front();
        g.TranslateTransform(BoardX+(head.x+.5f)*Unit,BoardY+(head.y+.5f)*Unit);
        g.RotateTransform(static_cast<float>(std::atan2(game.direction.y,game.direction.x)*180/3.141592653589793));
        ellipse(g,1,0,14,12.5f,0xA8D975);
        ellipse(g,5,-8,5.5f,6,0xFFFDF1);ellipse(g,5,8,5.5f,6,0xFFFDF1);
        if (game.state==State::Lost) for(float y : {-8.0f,8.0f}) {
            line(g,{3,y-2},{7,y+2},0x405731,1.8f);line(g,{7,y-2},{3,y+2},0x405731,1.8f);
        } else {
            ellipse(g,7,-8,2.5f,3,0x394D31);ellipse(g,7,8,2.5f,3,0x394D31);
            ellipse(g,7.5f,-9,1,1,0xFFFFFF);ellipse(g,7.5f,7,1,1,0xFFFFFF);
        }
        ellipse(g,3,-12,3,1.6f,0xEFB6A8);ellipse(g,3,12,3,1.6f,0xEFB6A8);
        GraphicsPath smile;smile.AddBezier(11,-3,15,-1,15,1,11,3);Pen pen(color(0x52713A),1.6f);g.DrawPath(&pen,&smile);
        if(game.state==State::Running && std::sin(frameTime*.003)>.88) {
            line(g,{14,0},{21,0},0xD98091,2);line(g,{21,0},{24,-2},0xD98091,2);line(g,{21,0},{24,2},0xD98091,2);
        }
        g.Restore(saved);
    }
    void paint(Graphics& g) {
        g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        SolidBrush bg(color(0xEDF6DF));g.FillRectangle(&bg,RectF(0,0,DesignW,DesignH));
        for(int y=12;y<840;y+=23)for(int x=12;x<940;x+=23)ellipse(g,float(x),float(y),1,1,0xD9E7C8);
        rounded(g,{25,22,68,68},22,0xFFFBCF,0xCFDBAC);mascot(g,35,41,.47f);
        text(g,L"B E R R Y   H A P P Y   S N A K E",{111,18,430,20},10,0x709061,true);
        text(g,L"莓莓蛇",{107,35,250,43},32,0x526E3D,true);
        text(g,L"咔嚓！再吃一口，把快乐养大一点。",{111,80,500,25},13,0x788769);
        rounded(g,{713,42,186,34},17,0xFFF4BE,0xE4D79B);text(g,L"今天也要莓烦恼",{713,42,186,34},12,0xA6853E,false,true);
        rounded(g,{23,119,595,684},24,0xC4D5AD);rounded(g,{23,112,595,684},24,0xFFFDF4,0x648747,3);
        const unsigned int statColors[]={0xF0F2E5,0xFFF0CA,0xF2EAFA};
        const wchar_t* statNames[]={L"本局分数",L"最高纪录",L"成长等级"};
        const int statValues[]={game.score,game.best,game.level};
        const unsigned int statInks[]={0x496637,0xAA8138,0x7961A8};
        for(int i=0;i<3;++i) {
            float x=40+i*190.0f;rounded(g,{x,127,180,47},12,statColors[i]);
            text(g,statNames[i],{x+12,134,77,28},11,0x7E8268);
            text(g,std::to_wstring(statValues[i]),{x+89,127,77,47},25,statInks[i],true,true);
        }
        rounded(g,{BoardX-4,BoardY-4,BoardW+8,BoardW+8},8,0x93AE6C);
        for(int y=0;y<20;++y)for(int x=0;x<20;++x) {
            SolidBrush brush(color((x+y)%2?0xDEEBBC:0xE7F0CC));
            g.FillRectangle(&brush,RectF(BoardX+x*Unit,BoardY+y*Unit,Unit+.1f,Unit+.1f));
        }
        for(const Cell& p : {Cell{2,2},Cell{17,3},Cell{3,16},Cell{16,17},Cell{1,10},Cell{13,1}})
            flower(g,BoardX+(p.x+.5f)*Unit,BoardY+(p.y+.5f)*Unit,0xF5D9CF);
        if(game.state!=State::Won)strawberry(g,BoardX+(game.food.x+.5f)*Unit,BoardY+(game.food.y+.5f)*Unit,.84f);
        drawSnake(g);
        button(g,{40,758,105,29},game.state==State::Paused?L"继续":L"暂停");
        button(g,{159,758,123,29},L"重新开始");text(g,L"空格暂停 · Enter 开始",{315,758,285,29},11,0x8B937F,false,true);

        rounded(g,{645,115,271,204},20,0xFFFDF4,0xD6DFC7);
        text(g,L"✿ 选个好心情",{665,130,230,33},18,0x69804F,true);
        const wchar_t* difficultyNames[]={L"慢悠悠 · 轻松",L"刚刚好 · 标准",L"冲冲冲 · 挑战"};
        for(int i=0;i<3;++i)button(g,{665,170+i*43.0f,230,32},difficultyNames[i],false,selectedDifficulty==i);
        text(g,L"切换难度后，下一局生效",{665,297,235,17},10,0x839174);
        rounded(g,{645,339,271,332},20,0xFFFDF4,0xD6DFC7);
        text(g,L"小蛇驾驶指南",{665,353,230,32},18,0x69804F,true);
        text(g,L"方向键 或 W A S D 移动\n空格暂停，Enter 开始 / 继续\n也可以点击下方方向按钮",{665,393,231,80},13,0x839174);
        button(g,{754,486,48,43},L"↑");button(g,{701,534,48,43},L"←");
        button(g,{807,534,48,43},L"→");button(g,{754,582,48,43},L"↓");
        text(g,L"✿",{754,534,48,43},23,0x94AE78,true,true);
        strawberry(g,681,644,.65f);text(g,L"一颗草莓 +10 分",{707,630,185,25},13,0xBA737E,true);
        rounded(g,{645,690,271,106},20,0xFFEFBF,0xE6D59D);
        text(g,L"☀ 花园小贴士",{665,698,230,32},17,0xA6853E,true);
        text(g,L"每吃 5 颗升一级，速度逐渐变快。\n避开边界和身体，不能直接掉头。",{665,735,230,47},11,0x9E8C61);
        text(g,L"C++ 小花园 · 无需浏览器 · 最高纪录保存在本机",{0,812,940,20},11,0x99A38C,false,true);
        if(game.state!=State::Running) {
            SolidBrush veil(color(0xEDF5D9,175));g.FillRectangle(&veil,RectF(BoardX,BoardY,BoardW,BoardW));
            rounded(g,{101,334,438,270},23,0xB6C99B);rounded(g,{101,327,438,270},23,0xFFFDF5,0x6A8950,3);
            mascot(g,266,348,.8f);
            std::wstring title,message,label;
            if(game.state==State::Ready) {title=L"小蛇准备开饭啦！";message=L"吃掉草莓，慢慢长大。\n小心别撞到边界和自己哦。";label=L"开始吃莓";}
            else if(game.state==State::Paused) {title=L"小蛇休息一会儿";message=L"草莓不会跑，准备好了再继续。";label=L"继续吃莓";}
            else {title=game.state==State::Won?L"花园被你吃满啦！":L"哎呀，撞到啦！";message=L"吃掉 "+std::to_wstring(game.score/10)+L" 颗草莓，获得 "+std::to_wstring(game.score)+L" 分。\n整理好心情，再来一口吧。";label=L"再玩一局";}
            text(g,title,{116,425,408,39},24,0x597C40,true,true);
            text(g,message,{118,465,404,57},13,0x859075,false,true);
            button(g,{229,540,182,40},label,true);
        }
    }
    bool hit(float x,float y,RectF r) {return x>=r.X&&y>=r.Y&&x<=r.GetRight()&&y<=r.GetBottom();}
    void click(float x,float y) {
        if(game.state!=State::Running&&hit(x,y,{229,540,182,40})) {
            if(game.state==State::Paused)togglePause();else startGame();return;
        }
        if(hit(x,y,{40,758,105,29}))togglePause();
        else if(hit(x,y,{159,758,123,29}))startGame();
        for(int i=0;i<3;++i)if(hit(x,y,{665,170+i*43.0f,230,32}))selectedDifficulty=i;
        const std::array<RectF,4> buttons={RectF(754,486,48,43),RectF(701,534,48,43),RectF(807,534,48,43),RectF(754,582,48,43)};
        const Cell dirs[]={{0,-1},{-1,0},{1,0},{0,1}};
        for(std::size_t i=0;i<buttons.size();++i)if(hit(x,y,buttons[i])) {
            if(game.state==State::Ready) startGame();
            game.turn(dirs[i]);
        }
        InvalidateRect(mainWindow,nullptr,FALSE);
    }
    LRESULT CALLBACK windowProcedure(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
        switch(message) {
        case WM_CREATE: SetTimer(window,1,16,nullptr);lastTime=GetTickCount64();return 0;
        case WM_ERASEBKGND:return 1;
        case WM_GETMINMAXINFO: {auto info=reinterpret_cast<MINMAXINFO*>(lParam);info->ptMinTrackSize={500,500};return 0;}
        case WM_SIZE:InvalidateRect(window,nullptr,FALSE);return 0;
        case WM_TIMER: {
            ULONGLONG now=GetTickCount64();int delta=static_cast<int>(std::min<ULONGLONG>(now-lastTime,100));lastTime=now;frameTime=now;
            if(game.state==State::Running) {
                accumulator+=delta;
                while(accumulator>=game.interval()&&game.state==State::Running) {
                    accumulator-=game.interval();game.step();saveRecord();
                }
                InvalidateRect(window,nullptr,FALSE);
            }
            return 0;
        }
        case WM_KEYDOWN: {
            Cell next{0,0};bool movement=true;
            switch(wParam) {
                case VK_UP:case 'W':next={0,-1};break;
                case VK_DOWN:case 'S':next={0,1};break;
                case VK_LEFT:case 'A':next={-1,0};break;
                case VK_RIGHT:case 'D':next={1,0};break;
                default:movement=false;break;
            }
            if(movement) {if(game.state==State::Ready)startGame();game.turn(next);}
            else if((wParam==VK_SPACE||wParam==VK_RETURN)&&!(lParam&(1LL<<30))) {
                if(game.state==State::Running||game.state==State::Paused)togglePause();else startGame();
            } else if(wParam==VK_ESCAPE)game.pause();
            InvalidateRect(window,nullptr,FALSE);return 0;
        }
        case WM_ACTIVATE: if(LOWORD(wParam)==WA_INACTIVE){game.pause();InvalidateRect(window,nullptr,FALSE);}return 0;
        case WM_LBUTTONDOWN:SetFocus(window);click((GET_X_LPARAM(lParam)-offsetX)/screenScale,(GET_Y_LPARAM(lParam)-offsetY)/screenScale);return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT bounds;GetClientRect(window,&bounds);
            int width=bounds.right,height=bounds.bottom;
            if(width>0&&height>0) {
                Bitmap bitmap(width,height,PixelFormat32bppPARGB);Graphics back(&bitmap);back.Clear(color(0xEDF6DF));
                screenScale=std::min(width/DesignW,height/DesignH);offsetX=(width-DesignW*screenScale)/2;offsetY=(height-DesignH*screenScale)/2;
                Matrix transform(screenScale,0,0,screenScale,offsetX,offsetY);back.SetTransform(&transform);paint(back);
                Graphics front(dc);front.DrawImage(&bitmap,0,0);
            }
            EndPaint(window,&ps);return 0;
        }
        case WM_DESTROY:KillTimer(window,1);saveRecord();PostQuitMessage(0);return 0;
        }
        return DefWindowProcW(window,message,wParam,lParam);
    }
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR commandLine,int show) {
    SetProcessDPIAware();GdiplusStartupInput input;
    if(GdiplusStartup(&gdiplusToken,&input,nullptr)!=Ok)return 1;
    // Render the same native drawing code offscreen for visual verification.
    // Usage: berry-snake.exe --preview "C:\\path\\preview.png"
    std::wstring arguments=commandLine?commandLine:L"";
    if(arguments.rfind(L"--preview ",0)==0) {
        std::wstring output=arguments.substr(10);
        if(output.size()>=2&&output.front()==L'"'&&output.back()==L'"')output=output.substr(1,output.size()-2);
        int result=1;
        {
            Bitmap image(static_cast<INT>(DesignW),static_cast<INT>(DesignH),PixelFormat32bppARGB);
            Graphics graphics(&image);game.start(1);paint(graphics);
            UINT count=0,bytes=0;
            if(GetImageEncodersSize(&count,&bytes)==Ok&&bytes>0) {
                std::vector<BYTE> storage(bytes);
                auto encoders=reinterpret_cast<ImageCodecInfo*>(storage.data());
                if(GetImageEncoders(count,bytes,encoders)==Ok)for(UINT i=0;i<count;++i)
                    if(std::wstring(encoders[i].MimeType)==L"image/png") {
                        result=image.Save(output.c_str(),&encoders[i].Clsid,nullptr)==Ok?0:1;break;
                    }
            }
        }
        GdiplusShutdown(gdiplusToken);return result;
    }
    DWORD saved=0,bytes=sizeof(saved);
    if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\BerrySnake",L"BestScore",RRF_RT_REG_DWORD,nullptr,&saved,&bytes)==ERROR_SUCCESS&&saved<=100000)
        game.best=savedBest=static_cast<int>(saved);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=windowProcedure;wc.hInstance=instance;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);wc.lpszClassName=L"BerrySnakeWindow";
    if(!RegisterClassExW(&wc)){GdiplusShutdown(gdiplusToken);return 1;}
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    const float scale=std::min({1.0f,(work.right-work.left-60)/DesignW,(work.bottom-work.top-80)/DesignH});
    RECT desired{0,0,static_cast<LONG>(DesignW*scale),static_cast<LONG>(DesignH*scale)};
    AdjustWindowRect(&desired,WS_OVERLAPPEDWINDOW,FALSE);
    mainWindow=CreateWindowExW(0,wc.lpszClassName,L"莓莓蛇 · C++ 卡通贪吃蛇",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,desired.right-desired.left,desired.bottom-desired.top,nullptr,nullptr,instance,nullptr);
    if(!mainWindow){GdiplusShutdown(gdiplusToken);return 1;}
    ShowWindow(mainWindow,show);UpdateWindow(mainWindow);
    MSG message{};int status;
    while((status=GetMessageW(&message,nullptr,0,0))>0){TranslateMessage(&message);DispatchMessageW(&message);}
    GdiplusShutdown(gdiplusToken);return status<0?1:static_cast<int>(message.wParam);
}
#endif
