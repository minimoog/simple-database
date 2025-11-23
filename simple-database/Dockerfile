FROM gcc:10

RUN apt update && \
    apt install -y \
        build-essential \
        libssl-dev \
        make \
        clang-tidy \
        clang-format \
        wget

# install cmake 3.16
WORKDIR /usr/src
RUN wget -qO- https://github.com/Kitware/CMake/releases/download/v3.16.9/cmake-3.16.9-linux-x86_64.tar.gz | tar -zxv

ENV PATH "/usr/src/cmake-3.16.9-Linux-x86_64/bin:$PATH"

WORKDIR /
