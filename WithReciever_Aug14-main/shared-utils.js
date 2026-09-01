// Shared utility functions for PCC S.T.A.T.S. system

/**
 * Get the device room ID from Firebase
 * This reads the device_room_id field that the ESP32 writes on startup
 * @param {Object} db - Firebase database reference
 * @returns {Promise<string>} The room ID (e.g., "Room_1", "Room_402")
 */
async function getDeviceRoomId(db) {
    try {
        const snapshot = await db.ref('.info/connected').once('value');
        if (!snapshot.val()) {
            console.log('[getDeviceRoomId] Firebase not connected, using default Room_1');
            return 'Room_1';
        }
        
        // Try to read device_room_id from any room (first room that has it)
        const roomsSnapshot = await db.ref().once('value');
        const data = roomsSnapshot.val();
        
        if (!data) {
            console.log('[getDeviceRoomId] No data in Firebase, using default Room_1');
            return 'Room_1';
        }
        
        // Look for device_room_id in any room
        for (const key of Object.keys(data)) {
            if (data[key] && data[key].device_room_id) {
                console.log(`[getDeviceRoomId] Found device_room_id in ${key}: ${data[key].device_room_id}`);
                return data[key].device_room_id;
            }
        }
        
        console.log('[getDeviceRoomId] No device_room_id found, using default Room_1');
        return 'Room_1';
    } catch (error) {
        console.error('[getDeviceRoomId] Error:', error);
        return 'Room_1';
    }
}

/**
 * Calculate Normal and Critical counts from room data
 * Uses individual room thresholds (maxT) for consistency across all pages
 * @param {Object} labsData - The room data object from Firebase
 * @param {Array} labKeys - Array of room keys
 * @returns {Object} { totalLabs, normalCount, criticalCount, avgTemp }
 */
function calculateStats(labsData, labKeys) {
    console.log('[calculateStats] ===== START =====');
    console.log('[calculateStats] Input labsData:', labsData);
    console.log('[calculateStats] Input labKeys:', labKeys);
    console.log('[calculateStats] labsData keys:', labsData ? Object.keys(labsData) : 'null/undefined');
    
    let sumT = 0;
    let criticalCount = 0;
    let count = 0;
    
    if (!labKeys || labKeys.length === 0) {
        console.log('[calculateStats] WARNING: labKeys is empty or null!');
        return { totalLabs: 0, normalCount: 0, criticalCount: 0, avgTemp: "--" };
    }
    
    labKeys.forEach(key => {
        const d = labsData[key];
        console.log(`[calculateStats] Processing room ${key}:`, d);
        if (!d) {
            console.log(`[calculateStats] Room ${key} has no data, skipping`);
            return;
        }
        
        const temp = parseFloat(d.temperature) || 0;
        // Handle both data structures: Dashboard uses thresholds.maxTemp, Analytics uses maxT directly
        const maxT = parseFloat(d.thresholds?.maxTemp) || parseFloat(d.maxT) || 30;
        console.log(`[calculateStats] Room ${key}: temp=${temp}, maxT=${maxT}, status=${d.status}`);
        sumT += temp;
        
        // Critical if status is ALARM or temperature exceeds individual room threshold
        if (d.status === "ALARM" || temp > maxT) {
            criticalCount++;
            console.log(`[calculateStats] Room ${key} marked as CRITICAL`);
        }
        
        count++;
    });
    
    const result = {
        totalLabs: count,
        normalCount: count - criticalCount,
        criticalCount: criticalCount,
        avgTemp: count > 0 ? (sumT / count).toFixed(1) + "°C" : "--"
    };
    
    console.log('[calculateStats] Output result:', result);
    console.log('[calculateStats] ===== END =====');
    return result;
}

/**
 * Update summary card elements with calculated stats
 * @param {Object} stats - Stats object from calculateStats()
 * @param {Object} elementIds - Object mapping stat types to element IDs
 */
function updateSummaryCards(stats, elementIds) {
    const ids = elementIds || {
        total: 'total-count',
        normal: 'normal-count',
        critical: 'alert-count',
        avgTemp: 'avg-temp'
    };
    
    if (document.getElementById(ids.total)) {
        document.getElementById(ids.total).innerText = stats.totalLabs;
    }
    if (document.getElementById(ids.normal)) {
        document.getElementById(ids.normal).innerText = stats.normalCount;
    }
    if (document.getElementById(ids.critical)) {
        document.getElementById(ids.critical).innerText = stats.criticalCount;
    }
    if (document.getElementById(ids.avgTemp)) {
        document.getElementById(ids.avgTemp).innerText = stats.avgTemp;
    }
}

/**
 * Popup notification system with number badge for multiple alerts
 * Auto-dismisses after 3 seconds
 */
class NotificationSystem {
    constructor() {
        this.notifications = [];
        this.container = null;
        this.init();
    }

    init() {
        // Create notification container
        this.container = document.createElement('div');
        this.container.id = 'notification-container';
        this.container.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 10000;
            display: flex;
            flex-direction: column;
            gap: 10px;
        `;
        document.body.appendChild(this.container);
    }

    show(message, type = 'info') {
        const id = Date.now();
        this.notifications.push({ id, message, type });
        this.render();
        
        // Auto-dismiss after 3 seconds
        setTimeout(() => {
            this.remove(id);
        }, 3000);
    }

    remove(id) {
        this.notifications = this.notifications.filter(n => n.id !== id);
        this.render();
    }

    render() {
        this.container.innerHTML = '';
        
        if (this.notifications.length === 0) return;

        // Show a single notification with badge count
        const latest = this.notifications[this.notifications.length - 1];
        const count = this.notifications.length;
        
        const notification = document.createElement('div');
        notification.style.cssText = `
            background: white;
            border-radius: 12px;
            padding: 16px 20px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.15);
            display: flex;
            align-items: center;
            gap: 12px;
            min-width: 300px;
            animation: slideIn 0.3s ease-out;
            border-left: 4px solid ${this.getColor(latest.type)};
        `;

        const badge = count > 1 ? `
            <div style="
                background: #ef4444;
                color: white;
                border-radius: 50%;
                width: 24px;
                height: 24px;
                display: flex;
                align-items: center;
                justify-content: center;
                font-size: 12px;
                font-weight: bold;
            ">${count}</div>
        ` : '';

        notification.innerHTML = `
            ${badge}
            <span style="font-size: 14px; font-weight: 500; color: #374151;">${latest.message}</span>
        `;

        this.container.appendChild(notification);
    }

    getColor(type) {
        const colors = {
            info: '#3b82f6',
            success: '#10b981',
            warning: '#f59e0b',
            error: '#ef4444'
        };
        return colors[type] || colors.info;
    }
}

// Initialize notification system globally when DOM is ready
let notificationSystem = null;

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        notificationSystem = new NotificationSystem();
    });
} else {
    // DOM is already ready
    notificationSystem = new NotificationSystem();
}

// Add animation keyframes
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(100%);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }
`;
if (document.head) {
    document.head.appendChild(style);
} else {
    document.addEventListener('DOMContentLoaded', () => {
        document.head.appendChild(style);
    });
}
