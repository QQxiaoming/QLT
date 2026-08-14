#! /bin/bash

CURRENT_PATH=$(cd "$(dirname "$0")";pwd)
PROJECT_DIR=$(basename "${CURRENT_PATH}")

CYAN="\033[1;36m"
NC="\033[0m"

echolog ()
{
    echo -e "\n${CYAN}[INFO]: ${1}${NC}\n"
}

echolog "Creating and starting container..."
CONTAINER_ID=`docker run -v "${CURRENT_PATH}":"/${PROJECT_DIR}" -itd qlt:latest`

echolog "Compiling in container..."
docker exec -it -w "/${PROJECT_DIR}/" "${CONTAINER_ID}" bash -c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target all"

echolog "Stopping container..."
docker stop "${CONTAINER_ID}"
echolog "Deleting container..."
docker rm "${CONTAINER_ID}"
