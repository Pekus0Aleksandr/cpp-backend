# Инициализация conan и cmake
```sh
mkdir build
cd build
conan install ..
cmake ..
```
# Сборка
В папке `build` выполнить команду
```sh
cmake --build .
```
# Запуск
В папке `build` выполнить команду
```sh
bin/game_server ../data/config.json ../static/
```
После этого можно открыть в браузере:
* http://127.0.0.1:8080/api/v1/maps для получения списка карт и
* http://127.0.0.1:8080/api/v1/map/map1 для получения подробной информации о карте `map1`
* http://127.0.0.1:8080/ для чтения статического контента (в каталоге static)

# NEW build
$ mkdir build_debug && cd build_debug

# Под Linux обязательно нужно указывать параметр
-s compiler.libcxx=libstdc++11. Иначе собранная
программа будет падать:
$ conan install .. -s compiler.libcxx=libstdc++11 -s build_type=Debug

$ cmake .. -DCMAKE_BUILD_TYPE=Debug

команда сборки и запуска
$ cmake --build . && bin/game_server

#### build in WIN ####
mkdir -p build 
cd build
conan install .. --build=missing -s build_type=Debug
conan install .. --build=missing -s build_type=Release
conan install .. --build=missing -s build_type=RelWithDebInfo
conan install .. --build=missing -s build_type=MinSizeRel
cmake ..

##### если при компоновке возникают сообщения о несовместимости
mkdir -p build 
cd build
conan install .. --build=missing -s build_type=Debug -s compiler.runtime=MD
conan install .. --build=missing -s build_type=Release -s compiler.runtime=MT
conan install .. --build=missing -s build_type=RelWithDebInfo -s compiler.runtime=MT
conan install .. --build=missing -s build_type=MinSizeRel -s compiler.runtime=MT
cmake .. 


##### Build in LINUX 
mkdir -p build-release 
cd build-release
conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Release
cd ..

mkdir -p build-debug
cd build-debug
conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Debug
cd .

# Запуск сервера 
sudo docker build -t server_logger .
sudo docker run --rm -it --entrypoint bash -p 80:8080 server_logger

Если в докер файле указан ENDPOINT
sudo docker run --rm -p 80:8080 server_logger
