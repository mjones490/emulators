FROM ubuntu:latest
RUN apt-get update && apt-get install -y \
    libsdl2-2.0-0 \
    libsdl2-mixer-2.0-0 \
    gdb \
    libgl1 \
    libglu1-mesa \
    libdrm2 \
    libxrandr2 \
    libxinerama1 \
    libxcursor1 \
    libxi6 \
    libx11-xcb1
WORKDIR /app
COPY . .
CMD "./dynosaur"
