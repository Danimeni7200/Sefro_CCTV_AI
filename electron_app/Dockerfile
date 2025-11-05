# Use Node.js 18 alpine image
FROM node:18-alpine

# Install dependencies for Electron
RUN apk add --no-cache \
    xvfb \
    mesa-gl \
    libx11 \
    libxrandr \
    libxext \
    libxi \
    libxcursor \
    libxinerama \
    libxss \
    libxtst \
    libxcomposite \
    libxdamage \
    libgtk-3.0 \
    alsa-lib \
    nss \
    && rm -rf /var/cache/apk/*

# Set working directory
WORKDIR /app

# Copy package files
COPY package*.json ./

# Install dependencies
RUN npm ci

# Copy application code
COPY . .

# Install Electron dependencies
RUN npm install electron

# Command to run the application with Xvfb (virtual framebuffer)
CMD ["xvfb-run", "--auto-servernum", "--server-args", "-screen 0 1024x768x24", "npm", "start"]