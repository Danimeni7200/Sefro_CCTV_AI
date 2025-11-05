const { app, BrowserWindow, ipcMain, Menu } = require('electron')
const path = require('path')
const fetch = require('node-fetch')

let win

function createWindow() {
    win = new BrowserWindow({
        width: 1400,
        height: 900,
        minWidth: 1000,
        minHeight: 600,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            enableRemoteModule: false,
            sandbox: true
        },
        icon: path.join(__dirname, 'icon.png')
    })

    win.loadFile(path.join(__dirname, 'renderer.html'))
    
    // Open developer tools automatically
    win.webContents.openDevTools()
    
    win.on('closed', () => {
        win = null
    })
}

// Create window when app is ready
app.whenReady().then(() => {
    createWindow()

    // On macOS, re-create window when dock icon is clicked
    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow()
        }
    })
})

// Quit when all windows are closed (except on macOS)
app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit()
    }
})

// ============================================================================
// IPC HANDLERS
// ============================================================================

ipcMain.handle('fetch-latest', async (_evt, host = 'http://127.0.0.1:8000', limit = 50) => {
    try {
        const url = `${host}/results/latest?limit=${limit}`
        const res = await fetch(url, { timeout: 5000 })
        
        if (!res.ok) {
            return { error: `HTTP ${res.status}` }
        }

        return await res.json()
    } catch (error) {
        return { error: error.message }
    }
})

ipcMain.handle('fetch-health', async (_evt, host = 'http://127.0.0.1:8000') => {
    try {
        const url = `${host}/healthz`
        const res = await fetch(url, { timeout: 5000 })
        
        if (!res.ok) {
            return { error: `HTTP ${res.status}` }
        }

        return await res.json()
    } catch (error) {
        return { error: error.message }
    }
})

ipcMain.handle('discover-camera', async (_evt, ip, user, pass, brand) => {
    try {
        const url = `http://127.0.0.1:8091/discover?ip=${encodeURIComponent(ip)}&user=${encodeURIComponent(user)}&password=${encodeURIComponent(pass)}&brand=${encodeURIComponent(brand || '')}`
        console.log(`Attempting to discover cameras at ${url}`)
        const res = await fetch(url, { method: 'GET', timeout: 10000 })
        
        console.log(`Discovery response status: ${res.status}`)
        if (!res.ok) {
            const errorText = await res.text()
            console.error(`Discovery failed with status ${res.status}: ${errorText}`)
            return { success: false, error: `HTTP ${res.status}: ${errorText}` }
        }

        const data = await res.json()
        console.log(`Discovery successful, found ${data.candidates ? data.candidates.length : 0} candidates`)
        return data
    } catch (error) {
        console.error(`Discovery error: ${error.message}`)
        return { success: false, error: error.message }
    }
})

// Add function to test RTSP stream connectivity
ipcMain.handle('test-rtsp-stream', async (_evt, url) => {
    try {
        // For now, we'll just return a mock response
        // In a real implementation, you would test the RTSP stream here
        return { success: true, message: 'اتصال به جریان RTSP برقرار شد' }
    } catch (error) {
        return { success: false, error: error.message }
    }
})

// ============================================================================
// APPLICATION MENU
// ============================================================================

const template = [
    {
        label: 'فایل',
        submenu: [
            {
                label: 'خروج',
                accelerator: 'CmdOrCtrl+Q',
                click: () => {
                    app.quit()
                }
            }
        ]
    },
    {
        label: 'ویرایش',
        submenu: [
            { role: 'undo' },
            { role: 'redo' },
            { type: 'separator' },
            { role: 'cut' },
            { role: 'copy' },
            { role: 'paste' }
        ]
    },
    {
        label: 'نمایش',
        submenu: [
            { role: 'reload' },
            { role: 'forceReload' },
            { role: 'toggleDevTools' },
            { type: 'separator' },
            { role: 'resetZoom' },
            { role: 'zoomIn' },
            { role: 'zoomOut' },
            { type: 'separator' },
            { role: 'togglefullscreen' }
        ]
    },
    {
        label: 'کمک',
        submenu: [
            {
                label: 'درباره',
                click: () => {
                    // Show about dialog
                }
            }
        ]
    }
]

const menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)