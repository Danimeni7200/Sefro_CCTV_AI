const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('lpr', {
    /**
     * Fetch latest license plate recognition results
     * @param {string} host - AI service host URL
     * @param {number} limit - Maximum number of results to fetch
     * @returns {Promise<Array>} Array of recognition results
     */
    fetchLatest: async (host, limit) => {
        try {
            return await ipcRenderer.invoke('fetch-latest', host, limit)
        } catch (e) {
            return { error: String(e) }
        }
    },

    /**
     * Check health status of AI service
     * @param {string} host - AI service host URL
     * @returns {Promise<Object>} Health status
     */
    fetchHealth: async (host) => {
        try {
            return await ipcRenderer.invoke('fetch-health', host)
        } catch (e) {
            return { error: String(e) }
        }
    },

    /**
     * Discover RTSP camera URLs
     * @param {string} ip - Camera IP address
     * @param {string} user - Camera username
     * @param {string} pass - Camera password
     * @param {string} brand - Camera brand (optional)
     * @returns {Promise<Object>} Discovery results with candidate URLs
     */
    discover: async (ip, user, pass, brand) => {
        try {
            return await ipcRenderer.invoke('discover-camera', ip, user, pass, brand)
        } catch (e) {
            return { success: false, error: String(e) }
        }
    }
})