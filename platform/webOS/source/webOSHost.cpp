
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <filesystem>
#include "httplib.h"
#include <iostream>

using namespace httplib;
namespace fs = std::filesystem;
using namespace std;

#include "../../SDL2Common/source/sdl2basehost.h"
#include "../../../source/hostVmShared.h"
#include "../../../source/nibblehelpers.h"
#include "../../../source/filehelpers.h"
#include "../../../source/logger.h"

#include "../../../source/emojiconversion.h"

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

/*#define WINDOW_SIZE_X 1280
#define WINDOW_SIZE_Y 720*/


#define WINDOW_FLAGS 0

#define RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

SDL_Event event;
const int JOYSTICK_DEAD_ZONE = 8000;


const std::string executable_path = "/media/developer/apps/usr/palm/applications/com.fake.fake8";

SDL_GameController *findController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

inline SDL_JoystickID getControllerInstanceID(SDL_GameController *controller) {
    return SDL_JoystickInstanceID(
            SDL_GameControllerGetJoystick(controller));
}




const char *upload_folder = "/media/developer/apps/usr/palm/applications/com.fake.fake8/p8carts"; // Hardcoded path for upload files

const char *html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File Upload</title>
    <script src="https://unpkg.com/htmx.org@1.6.1"></script>
</head>
<body>
    <form id="formElem">
        <input type="file" name="file" accept=".p8">
        <input type="submit">
    </form>
    <div id="fileList" hx-get="/files" hx-trigger="load"></div>
    <script>
        formElem.onsubmit = async (e) => {
            e.preventDefault();
            let res = await fetch('/upload', {
                method: 'POST',
                body: new FormData(formElem)
            });
            document.getElementById('fileList').innerHTML = await res.text();
            htmx.trigger(document.getElementById('fileList'), 'load');
        };
    </script>
</body>
</html>
)";

void list_files(Response &res) {
    string file_list_html = "<ul>";
    for (const auto &entry : fs::directory_iterator(upload_folder)) {
        string filename = entry.path().filename().string();
        file_list_html += "<li>" + filename + " <button hx-delete='/delete?file=" + filename + "' hx-swap='outerHTML'>Delete</button></li>";
    }
    file_list_html += "</ul>";
    res.set_content(file_list_html, "text/html");
}

string get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    string local_ip;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0) {
                if (string(ifa->ifa_name) != "lo") {
                    local_ip = host;
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
    return local_ip;
}

void runServer()
{
    fs::create_directory(upload_folder);

    Server svr;

    svr.Get("/", [](const Request & /*req*/, Response &res) {
        res.set_content(html, "text/html");
    });

    svr.Get("/files", [](const Request & /*req*/, Response &res) {
        list_files(res);
    });

    svr.Post("/upload", [](const Request &req, Response &res) {
        auto file = req.get_file_value("file");

        if (file.filename.substr(file.filename.find_last_of(".") + 1) == "p8") {
            string file_path = string(upload_folder) + "/" + file.filename;
            ofstream ofs(file_path, ios::binary);
            ofs << file.content;
            ofs.close();
        }

        list_files(res);
    });

    svr.Delete("/delete", [](const Request &req, Response &res) {
        string filename = req.get_param_value("file");
        string file_path = string(upload_folder) + "/" + filename;

        if (fs::exists(file_path)) {
            fs::remove(file_path);
        }

        list_files(res);
    });

    svr.listen("0.0.0.0", 8585);
}

void startServerInThread() {
    std::thread server_thread(runServer);
    server_thread.detach();
}


string _desktopSdl2SettingsDir = "fake08";
string _desktopSdl2SettingsPrefix = "fake08/";
string _desktopSdl2customBiosLua =
        "cartpath = \"~/p8carts/\"\n"
        "selectbtn = \"z\"\n"
        "pausebtn = \"esc\"\n"
        "exitbtn = \"close window\"\n"
        "sizebtn = \"\"";

Host::Host() 
{

    SDL_DisplayMode current;
    startServerInThread();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    SDL_JoystickEventState(SDL_ENABLE);
    int should_be_zero = SDL_GetCurrentDisplayMode(0, &current);
    controller = findController();

    if(should_be_zero != 0) {
      // In case of error...
      SDL_Log("Could not get display mode for video display #%d: %s", 0, SDL_GetError());
    }

    else {
      // On success, print the current display mode.
      SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.", 0, current.w, current.h, current.refresh_rate);
    }

    int WINDOW_SIZE_X=current.w;
    int WINDOW_SIZE_Y=current.h;

    struct stat st = {0};

    int res = chdir(executable_path.c_str());
    if (res == 0 && stat(_desktopSdl2SettingsDir.c_str(), &st) == -1) {
        res = mkdir(_desktopSdl2SettingsDir.c_str(), 0777);
    }
    
    string cartdatadir = _desktopSdl2SettingsPrefix + "cdata";
    if (res == 0 && stat(cartdatadir.c_str(), &st) == -1) {
        res = mkdir(cartdatadir.c_str(), 0777);
    }
	
    std::string fullCartDir = executable_path + "/p8carts";

    setPlatformParams(
        WINDOW_SIZE_X,
        WINDOW_SIZE_Y,
        WINDOW_FLAGS,
        RENDERER_FLAGS,
        PIXEL_FORMAT,
        _desktopSdl2SettingsPrefix,
        _desktopSdl2customBiosLua,
        fullCartDir
    );
}


InputState_t Host::scanInput() {
    currKDown = 0;
    uint8_t kUp = 0;
    stretchKeyPressed = false;
    uint8_t mouseBtnState = 0;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_CONTROLLERDEVICEADDED:
                if (!controller) {
                    controller = SDL_GameControllerOpen(event.cdevice.which);
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (controller && event.cdevice.which == getControllerInstanceID(controller)) {
                    SDL_GameControllerClose(controller);
                    controller = findController();
                }
                break;
            case SDL_CONTROLLERBUTTONDOWN :
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_START:  
                        currKDown |= P8_KEY_PAUSE; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  
                        currKDown |= P8_KEY_LEFT; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: 
                        currKDown |= P8_KEY_RIGHT; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:    
                        currKDown |= P8_KEY_UP; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  
                        currKDown |= P8_KEY_DOWN; 
                        break;
                    case SDL_CONTROLLER_BUTTON_A:     
                        currKDown |= P8_KEY_X; 
                        break;
                    case SDL_CONTROLLER_BUTTON_B:     
                        currKDown |= P8_KEY_O; 
                        break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: 
                        lDown = true; 
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: 
                        rDown = true; 
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK: 
                        stretchKeyPressed = true; 
                        break;
                }
                break;

            case SDL_CONTROLLERBUTTONUP:
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_START:  
                        kUp |= P8_KEY_PAUSE; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  
                        kUp |= P8_KEY_LEFT; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: 
                        kUp |= P8_KEY_RIGHT; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:    
                        kUp |= P8_KEY_UP; 
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  
                        kUp |= P8_KEY_DOWN; 
                        break;
                    case SDL_CONTROLLER_BUTTON_A:     
                        kUp |= P8_KEY_X; 
                        break;
                    case SDL_CONTROLLER_BUTTON_B:     
                        kUp |= P8_KEY_O; 
                        break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: 
                        lDown = false; 
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: 
                        rDown = false; 
                        break;
                }
               break;
            
            case SDL_QUIT:
                quit = 1;
                break;
        }
    }

    if (lDown && rDown){
        quit = 1;
    }

    currKHeld |= currKDown;
    currKHeld ^= kUp;

    return InputState_t {
        currKDown,
        currKHeld
    };
}

vector<string> Host::listcarts(){
    vector<string> carts;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (_cartDirectory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (isCartFile(ent->d_name)){
                carts.push_back(ent->d_name);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
    }
    
    return carts;
}

