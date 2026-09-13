// Shared utility functions for PCC S.T.A.T.S. system

/**
 * Returns the human-friendly display name for a room, UI-only (does not
 * affect the Firebase key or any lookup logic).
 *
 * Room_1 is special-cased to "Room 401": it was set up under the key
 * "Room_1" before being physically wired to the real Room 401, so its
 * Firebase key doesn't match its actual room number. Every other room
 * falls back to its stored name, or the key with underscores turned into
 * spaces (e.g. "Room_402" -> "Room 402").
 * @param {string} roomKey - The Firebase room key, e.g. "Room_1"
 * @param {Object} [roomData] - The room's data node, if available
 * @returns {string}
 */
function getRoomDisplayName(roomKey, roomData) {
    if (roomData && (roomData.customRoomName || roomData.roomName)) {
        return roomData.customRoomName || roomData.roomName;
    }
    if (roomKey === 'Room_1') return 'Room 401';
    return String(roomKey || '').replace(/_/g, ' ');
}

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
 * Popup notification (toast) system - stacks multiple visible notifications,
 * each dismissible on its own, styled to match the app's maroon/neumorphic
 * design system.
 *
 * watchFirebaseAlerts(db) is what makes notifications actually consistent
 * across pages: previously every page loaded this file, but only the
 * Dashboard ever called notificationSystem.show() - so a device going
 * offline or a room becoming occupied only surfaced a popup if you happened
 * to have the Dashboard tab open. Calling watchFirebaseAlerts(db) once on
 * any page (Dashboard, Analytics, Alerts, Settings all do) subscribes that
 * page to the same shared alerts/ Firebase node, so a toast now appears
 * wherever you actually are when a new alert is created.
 */
const NOTIFICATION_ICONS = {
    success: '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path><polyline points="22 4 12 14.01 9 11.01"></polyline></svg>',
    warning: '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path><line x1="12" y1="9" x2="12" y2="13"></line><line x1="12" y1="17" x2="12.01" y2="17"></line></svg>',
    error: '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="7.86 2 16.14 2 22 7.86 22 16.14 16.14 22 7.86 22 2 16.14 2 7.86 7.86 2"></polygon><line x1="12" y1="8" x2="12" y2="12"></line><line x1="12" y1="16" x2="12.01" y2="16"></line></svg>',
    info: '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>'
};

const NOTIFICATION_COLORS = {
    success: '#10B981',
    warning: '#F59E0B',
    error: '#800000',
    info: '#3B82F6'
};

const MAX_VISIBLE_NOTIFICATIONS = 4;

class NotificationSystem {
    constructor() {
        this.entries = [];
        this.nextId = 1;
        this.container = null;
        this.watchedDbs = new Set();
        this.init();
    }

    init() {
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
            max-width: 360px;
            pointer-events: none;
        `;
        document.body.appendChild(this.container);
    }

    show(message, type = 'info') {
        const id = this.nextId++;
        const el = this.buildElement(id, message, type);
        this.container.appendChild(el);
        this.entries.push({ id, el });

        // Keep the stack from growing without bound if alerts arrive quickly
        while (this.entries.length > MAX_VISIBLE_NOTIFICATIONS) {
            const oldest = this.entries.shift();
            this.dismissElement(oldest.el, true);
        }

        const lifespan = (type === 'error' || type === 'warning') ? 5000 : 3500;
        el._dismissTimer = setTimeout(() => this.remove(id), lifespan);
    }

    remove(id) {
        const idx = this.entries.findIndex(n => n.id === id);
        if (idx === -1) return;
        const { el } = this.entries[idx];
        this.entries.splice(idx, 1);
        this.dismissElement(el, false);
    }

    dismissElement(el, immediate) {
        clearTimeout(el._dismissTimer);
        if (immediate) {
            el.remove();
            return;
        }
        el.style.animation = 'toast-slide-out 0.2s ease-in forwards';
        setTimeout(() => el.remove(), 200);
    }

    buildElement(id, message, type) {
        const color = NOTIFICATION_COLORS[type] || NOTIFICATION_COLORS.info;
        const icon = NOTIFICATION_ICONS[type] || NOTIFICATION_ICONS.info;

        // Solid, fully opaque background - NOT var(--bg-card), which in this
        // app's design system is a translucent rgba(255,255,255,0.4) glass
        // token meant for cards sitting in normal page flow. Used here with
        // no backdrop-filter, it let whatever was behind the toast show
        // straight through, making it look like it overlapped the page.
        const el = document.createElement('div');
        el.style.cssText = `
            background: #ffffff;
            border: 1px solid rgba(0, 0, 0, 0.06);
            border-radius: var(--radius-lg, 16px);
            padding: 14px 14px 14px 16px;
            box-shadow: var(--shadow-lg, 0 10px 25px rgba(0,0,0,0.2));
            display: flex;
            align-items: flex-start;
            gap: 12px;
            min-width: 300px;
            border-left: 4px solid ${color};
            font-family: var(--font-family, 'Inter', -apple-system, sans-serif);
            animation: toast-slide-in 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            pointer-events: auto;
        `;

        const iconWrap = document.createElement('div');
        iconWrap.style.cssText = `flex-shrink: 0; width: 20px; height: 20px; margin-top: 1px; color: ${color};`;
        iconWrap.innerHTML = icon;

        const text = document.createElement('div');
        text.style.cssText = `flex: 1; font-size: 13px; font-weight: 600; color: #1E293B; line-height: 1.4;`;
        text.textContent = message;

        const closeBtn = document.createElement('button');
        closeBtn.setAttribute('aria-label', 'Dismiss notification');
        closeBtn.style.cssText = `
            flex-shrink: 0; border: none; background: transparent; color: #94A3B8;
            cursor: pointer; font-size: 18px; line-height: 1; padding: 0 0 0 4px;
        `;
        closeBtn.textContent = '×';
        closeBtn.onclick = () => this.remove(id);

        el.appendChild(iconWrap);
        el.appendChild(text);
        el.appendChild(closeBtn);
        return el;
    }

    // Subscribe this page to Firebase's shared alerts/ node so any new
    // device-offline or occupancy-change alert shows a toast here too, not
    // just on the Dashboard. Only alerts created AFTER this call (i.e. while
    // this page is open) trigger a toast - the existing backlog of
    // unacknowledged alerts is left for the Alerts page's incident list to
    // show, not replayed as a flood of popups on every page load.
    watchFirebaseAlerts(db) {
        if (!db || typeof db.ref !== 'function' || this.watchedDbs.has(db)) return;
        this.watchedDbs.add(db);

        const startedAt = Date.now();
        db.ref('alerts').orderByChild('timestamp').startAt(startedAt).on('child_added', (snap) => {
            const alert = snap.val();
            if (!alert || alert.acknowledged) return;

            let type = 'info';
            if (alert.type === 'device_offline') {
                type = 'error';
            } else if (alert.type === 'occupancy_change') {
                type = alert.occupancyState === 'occupied' ? 'warning' : 'info';
            }

            this.show(alert.title || alert.message || 'New alert', type);
        });
    }
}

// Initialize notification system globally when DOM is ready
let notificationSystem = null;

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        notificationSystem = new NotificationSystem();
        window.notificationSystem = notificationSystem;
    });
} else {
    // DOM is already ready
    notificationSystem = new NotificationSystem();
    window.notificationSystem = notificationSystem;
}

// Add animation keyframes
const style = document.createElement('style');
style.textContent = `
    @keyframes toast-slide-in {
        from { transform: translateX(110%); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }
    @keyframes toast-slide-out {
        from { transform: translateX(0); opacity: 1; }
        to { transform: translateX(110%); opacity: 0; }
    }
`;
if (document.head) {
    document.head.appendChild(style);
} else {
    document.addEventListener('DOMContentLoaded', () => {
        document.head.appendChild(style);
    });
}
