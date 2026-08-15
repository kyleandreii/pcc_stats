// Shared utility functions for PCC S.T.A.T.S. system

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
