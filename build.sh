arm-rockchip830-linux-uclibcgnueabihf-g++ src/server.cpp -o build/server -O3 -std=c++11 -Wall -Wextra -pthread
arm-rockchip830-linux-uclibcgnueabihf-g++ src/ipc_main.cpp -o build/ipc_main -O3 -std=c++14
arm-rockchip830-linux-uclibcgnueabihf-g++ src/ipc_client.cpp -o build/ipc_client -O3 -std=c++14
arm-rockchip830-linux-uclibcgnueabihf-g++ src/json_demo.cpp -o build/json_demo -O3 -std=c++11 -Wno-psabi