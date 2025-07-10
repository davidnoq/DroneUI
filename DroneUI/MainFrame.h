#pragma once

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <thread>
#include <atomic>
#include "Drone.h"

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SocketType;
#endif

class MainFrame : public wxFrame {
private:
    // — Drone simulation state —
    Drone drone;
    std::atomic<bool> simRunning{ false };
    double targetX{ 0.0 };
    double targetY{ 0.0 };

    // — Networking state —
    SocketType sockfd;
    std::atomic<bool> netRunning{ false };
    std::thread    netThread;

    // — UI elements —
    wxPanel* mapPanel;
    wxStaticText* speedLabel;
    wxStaticText* xLabel;
    wxStaticText* yLabel;
    wxStaticText* orientationLabel;
    wxStaticText* accelLabel;
    wxButton* startBtn;
    wxButton* stopBtn;

    // — Event handlers —
    void OnPaintMap(wxPaintEvent& event);
    void OnConnectDrone(wxCommandEvent& event);
    void OnStartSimulation(wxCommandEvent& event);
    void OnStopSimulation(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void NetworkingLoop();

public:
    MainFrame(const wxString& title);
};
