#include "MainFrame.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <wx/log.h>
#include <wx/dcbuffer.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SocketType;
#endif

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 700)),
    drone(0.0, 0.0, 50.0, 0.0)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // — map panel —
    mapPanel = new wxPanel(this, wxID_ANY);
    mapPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    mapPanel->SetBackgroundColour(*wxLIGHT_GREY);
    mainSizer->Add(mapPanel, 1, wxEXPAND | wxALL, 5);
    mapPanel->Bind(wxEVT_PAINT, &MainFrame::OnPaintMap, this);

    // — telemetry —
    wxBoxSizer* tel = new wxBoxSizer(wxHORIZONTAL);
    speedLabel = new wxStaticText(this, wxID_ANY, "Speed: 0.00");
    xLabel = new wxStaticText(this, wxID_ANY, "AX:    0.00");
    yLabel = new wxStaticText(this, wxID_ANY, "AY:    0.00");
    orientationLabel = new wxStaticText(this, wxID_ANY, "Ori:   0.0°");
    accelLabel = new wxStaticText(this, wxID_ANY, "AZ:    0.00");
    tel->Add(speedLabel, 1, wxALL, 5);
    tel->Add(xLabel, 1, wxALL, 5);
    tel->Add(yLabel, 1, wxALL, 5);
    tel->Add(orientationLabel, 1, wxALL, 5);
    tel->Add(accelLabel, 1, wxALL, 5);
    mainSizer->Add(tel, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    // — buttons —
    wxBoxSizer* btns = new wxBoxSizer(wxHORIZONTAL);
    wxButton* connectBtn = new wxButton(this, wxID_ANY, "Connect Drone");
    startBtn = new wxButton(this, wxID_HIGHEST + 1, "Start Simulation");
    stopBtn = new wxButton(this, wxID_HIGHEST + 2, "Stop Simulation");
    stopBtn->Disable();
    wxButton* exitBtn = new wxButton(this, wxID_EXIT, "Exit");

    btns->Add(connectBtn, 1, wxALL, 5);
    btns->Add(startBtn, 1, wxALL, 5);
    btns->Add(stopBtn, 1, wxALL, 5);
    btns->Add(exitBtn, 1, wxALL, 5);
    mainSizer->Add(btns, 0, wxEXPAND | wxALL, 10);

    SetSizer(mainSizer);
    Centre();

    connectBtn->Bind(wxEVT_BUTTON, &MainFrame::OnConnectDrone, this);
    startBtn->Bind(wxEVT_BUTTON, &MainFrame::OnStartSimulation, this);
    stopBtn->Bind(wxEVT_BUTTON, &MainFrame::OnStopSimulation, this);
    Bind(wxEVT_BUTTON, &MainFrame::OnExit, this, wxID_EXIT);
}

void MainFrame::OnPaintMap(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(mapPanel);
    dc.Clear();

    int w, h;
    mapPanel->GetClientSize(&w, &h);
    int ox = w / 2, oy = h / 2;

    // draw crosshairs
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawLine(0, oy, w, oy);
    dc.DrawLine(ox, 0, ox, h);

    // compute cube
    double speed = drone.GetSpeed();
    double side = 20 + std::min(speed, 200.0) * 0.1;
    double half = side / 2, o3 = side * 0.3;
    double a = drone.GetOrientation() * M_PI / 180.0;
    double c = cos(a), s = sin(a);

    struct P { double x, y; };
    P pts[8] = {
      {-half,-half}, {half,-half}, {half,half}, {-half,half},
      {-half + o3,-half - o3}, {half + o3,-half - o3},
      {half + o3, half - o3}, {-half + o3, half - o3}
    };

    wxPoint dp[8];
    for (int i = 0; i < 8; i++) {
        double rx = pts[i].x * c - pts[i].y * s;
        double ry = pts[i].x * s + pts[i].y * c;
        dp[i].x = int(ox + drone.GetX() + rx);
        dp[i].y = int(oy - drone.GetY() + ry);
    }

    dc.SetBrush(*wxBLUE_BRUSH);
    dc.SetPen(*wxBLACK_PEN);
    // back face
    dc.DrawPolygon(4, &dp[4]);
    // front face
    dc.DrawPolygon(4, dp);
    // edges
    for (int i = 0; i < 4; i++) dc.DrawLine(dp[i], dp[i + 4]);
}

void MainFrame::OnConnectDrone(wxCommandEvent&)
{
    if (netRunning) {
        wxLogMessage("Already connected to drone.");
        return;
    }

    // reset position/orientation
    drone = Drone(0.0, 0.0, 0.0, 0.0);
    mapPanel->Refresh();

    const char* IP = "192.168.1.232";
    const int   PORT = 5000;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0
#ifdef _WIN32
        || sockfd == INVALID_SOCKET
#endif
        ) {
        wxLogError("Socket creation failed");
        return;
    }

    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
#ifdef _WIN32
    srv.sin_addr.s_addr = inet_addr(IP);
#else
    inet_pton(AF_INET, IP, &srv.sin_addr);
#endif

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) == 0) {
        wxLogMessage("Connected to drone at %s:%d", IP, PORT);
        netRunning = true;
        netThread = std::thread(&MainFrame::NetworkingLoop, this);
        netThread.detach();
    }
    else {
        wxLogError("Unable to connect to drone");
#ifdef _WIN32
        closesocket(sockfd);
#else
        ::close(sockfd);
#endif
    }
}

void MainFrame::NetworkingLoop()
{
    char buf[256];
    size_t idx = 0;
    while (netRunning) {
        // read one byte
#ifdef _WIN32
        int ret = recv(sockfd, &buf[idx], 1, 0);
#else
        ssize_t ret = recv(sockfd, &buf[idx], 1, 0);
#endif
        if (ret <= 0) break;

        // line complete?
        if (buf[idx] == '\n' || idx >= sizeof(buf) - 2) {
            buf[idx] = '\0';
            double px, py, speed, ori;
            // expect 4 comma-separated floats
            if (sscanf(buf, "%lf,%lf,%lf,%lf",
                &px, &py, &speed, &ori) == 4)
            {
                wxTheApp->CallAfter([=]() {
                    // update drone with position, speed, orientation
                    drone = Drone(px, py, speed, ori);

                    // update labels
                    speedLabel->SetLabel(
                        wxString::Format("Speed: %.2f", speed));
                    xLabel->SetLabel(
                        wxString::Format("X:     %.2f", px));
                    yLabel->SetLabel(
                        wxString::Format("Y:     %.2f", py));
                    orientationLabel->SetLabel(
                        wxString::Format("Ori:   %.1f°", ori));
                    accelLabel->SetLabel("AZ:    N/A");

                    // redraw the panel
                    mapPanel->Refresh();
                    });
            }
            idx = 0;  // reset buffer
        }
        else {
            ++idx;
        }
    }

    // clean up
    netRunning = false;
#ifdef _WIN32
    closesocket(sockfd);
#else
    ::close(sockfd);
#endif
    wxTheApp->CallAfter([&]() {
        wxLogMessage("Drone connection closed");
        });
}



void MainFrame::OnStartSimulation(wxCommandEvent& event)
{
    drone = Drone(0.0, 0.0, 50.0, 0.0);
    simRunning = true;
    startBtn->Disable(); stopBtn->Enable();

    std::thread([this]() {
        const double dt = 0.02;
        auto delay = std::chrono::duration<double>(dt);
        double x = 0, y = 0, speed = 50, ori = 0, accel = 0;
        const double turnResp = 5.0, accelGain = 20.0, decelGain = 15.0, maxSpeed = 300.0;

        while (simRunning) {
            double dx = targetX - x;
            double dy = targetY - y;
            double desired = atan2(dy, dx) * 180.0 / M_PI;
            if (desired < 0) desired += 360;
            double diff = desired - ori;
            if (diff > 180) diff -= 360;
            if (diff < -180) diff += 360;
            ori += diff * turnResp * dt;
            if (ori < 0) ori += 360;
            if (ori >= 360) ori -= 360;
            if (fabs(diff) < 5.0) accel += accelGain * dt; else accel -= decelGain * dt;
            accel = std::max(-decelGain, std::min(accelGain, accel));
            speed += accel * dt; speed = std::max(0.0, std::min(maxSpeed, speed));
            double r = ori * M_PI / 180.0;
            x += cos(r) * speed * dt; y += sin(r) * speed * dt;
            drone = Drone(x, y, speed, ori);
            wxTheApp->CallAfter([=]() {
                mapPanel->Refresh();
                speedLabel->SetLabel(wxString::Format("Speed: %.1f", speed));
                xLabel->SetLabel(wxString::Format("X: %.1f", x));
                yLabel->SetLabel(wxString::Format("Y: %.1f", y));
                orientationLabel->SetLabel(wxString::Format("Ori: %.0f°", ori));
                accelLabel->SetLabel(wxString::Format("Accel: %.1f", accel));
                });
            std::this_thread::sleep_for(delay);
        }
        }).detach();
}

void MainFrame::OnStopSimulation(wxCommandEvent& event)
{
    simRunning = false;
    stopBtn->Disable(); startBtn->Enable();
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    simRunning = false;
    if (netRunning) netRunning = false;
    Close(true);
}

void MainFrame::OnMouseMove(wxMouseEvent& evt)
{
    int w, h;
    mapPanel->GetClientSize(&w, &h);
    int ox = w / 2, oy = h / 2;
    auto p = evt.GetPosition();
    targetX = p.x - ox;
    targetY = oy - p.y;
    evt.Skip();
}
