(function () {
    const LABORATORY_OPTIONS = [
        'Laboratory 1',
        'Laboratory 2',
        'Laboratory 3',
        'Laboratory 4',
        'Laboratory 5',
        'Laboratory 6'
    ];

    const COMMAND_OPTIONS = [
        { key: 'acOn', label: 'AC ON', group: 'Power Control' },
        { key: 'acOff', label: 'AC OFF', group: 'Power Control' },
        { key: 'temperatureUp', label: 'Temperature UP', group: 'Temperature Control' },
        { key: 'temperatureDown', label: 'Temperature DOWN', group: 'Temperature Control' }
    ];

    const CONFIG = {
        apiKey: 'AIzaSyAv9fvbotQfi21Nu7LNZYBuzNUJOD5Vj0E',
        databaseURL: 'https://pcc-stats-default-rtdb.asia-southeast1.firebasedatabase.app/'
    };

    function ensureFirebase() {
        if (!window.firebase) {
            console.warn('Firebase SDK is not loaded.');
            return null;
        }
        if (!firebase.apps.length) {
            firebase.initializeApp(CONFIG);
        }
        return firebase;
    }

    function getDb() {
        const firebaseLib = ensureFirebase();
        return firebaseLib ? firebaseLib.database() : null;
    }

    function getAuth() {
        const firebaseLib = ensureFirebase();
        return firebaseLib && firebaseLib.auth ? firebaseLib.auth() : null;
    }

    function getCurrentUser() {
        const auth = getAuth();
        return auth ? auth.currentUser : null;
    }

    function isAuthorizedIRUser() {
        const user = getCurrentUser();
        if (!user || !user.email) return false;
        return user.email.toLowerCase().endsWith('@pccnet.edu.ph') || user.email.toLowerCase().includes('admin');
    }

    function getLabKey(labName) {
        return String(labName || 'Laboratory 1')
            .toLowerCase()
            .replace(/[^a-z0-9]+/g, '') || 'laboratory1';
    }

    function makeSignalString(entry) {
        const rawData = Array.isArray(entry.rawData) ? entry.rawData : [];
        if (!rawData.length) {
            return 'uint16_t rawData[] = {\n    0\n};';
        }
        const chunks = rawData.map((value) => Number(value) || 0);
        return `uint16_t rawData[] = {\n    ${chunks.join(', ')}\n};`;
    }

    function getCommandMeta(commandKey) {
        return COMMAND_OPTIONS.find((item) => item.key === commandKey) || {
            key: commandKey,
            label: commandKey,
            group: 'General'
        };
    }

    function normalizeRawData(rawValue) {
        if (Array.isArray(rawValue)) {
            return rawValue.map((value) => Number(value) || 0);
        }
        if (typeof rawValue === 'string') {
            const numbers = rawValue
                .replace(/uint16_t\s*rawData\s*\[\]\s*=\s*\{?|\};?$/gi, '')
                .split(/[\s,]+/)
                .map((part) => part.trim())
                .filter(Boolean)
                .map((part) => Number(part))
                .filter((part) => !Number.isNaN(part));
            return numbers;
        }
        return [];
    }

    function buildLibraryEntry(payload) {
        const rawData = normalizeRawData(payload.rawData || []);
        const commandKey = payload.commandKey || 'acOn';
        const commandMeta = getCommandMeta(commandKey);

        return {
            labName: payload.labName || 'Laboratory 1',
            commandKey,
            commandLabel: payload.commandLabel || commandMeta.label,
            protocol: payload.protocol || 'PANASONIC',
            signalName: payload.signalName || commandMeta.label,
            rawData,
            signalLength: Number(payload.signalLength || rawData.length || 0),
            createdAt: payload.createdAt || new Date().toISOString(),
            updatedAt: payload.updatedAt || new Date().toISOString(),
            note: payload.note || ''
        };
    }

    async function fetchIRLibrary() {
        const db = getDb();
        if (!db) return {};
        const snapshot = await db.ref('irRemoteLibrary').once('value');
        return snapshot.val() || {};
    }

    async function saveIRSignal(payload) {
        const db = getDb();
        if (!db) {
            throw new Error('Firebase database is unavailable.');
        }
        if (!isAuthorizedIRUser()) {
            throw new Error('Only authorized users can save or edit IR signals.');
        }

        const entry = buildLibraryEntry(payload);
        const labKey = getLabKey(entry.labName);
        const commandKey = entry.commandKey;

        const existingRef = db.ref(`irRemoteLibrary/${labKey}/${commandKey}`);
        const existingSnap = await existingRef.once('value');
        const existing = existingSnap.val();

        if (existing && !payload.forceReplace) {
            throw new Error('EXISTS');
        }

        const writePayload = {
            name: entry.signalName,
            protocol: entry.protocol,
            rawData: entry.rawData,
            signalLength: entry.signalLength,
            commandKey,
            commandLabel: entry.commandLabel,
            createdAt: existing?.createdAt || entry.createdAt,
            updatedAt: new Date().toISOString()
        };

        await existingRef.set(writePayload);
        return writePayload;
    }

    async function deleteIRSignal(labName, commandKey) {
        const db = getDb();
        if (!db) throw new Error('Firebase database is unavailable.');
        if (!isAuthorizedIRUser()) throw new Error('Only authorized users can delete IR signals.');
        await db.ref(`irRemoteLibrary/${getLabKey(labName)}/${commandKey}`).remove();
    }

    async function updateIRSignal(labName, commandKey, payload) {
        const db = getDb();
        if (!db) throw new Error('Firebase database is unavailable.');
        if (!isAuthorizedIRUser()) throw new Error('Only authorized users can edit IR signals.');

        const existingSnap = await db.ref(`irRemoteLibrary/${getLabKey(labName)}/${commandKey}`).once('value');
        const existing = existingSnap.val() || {};

        const nextPayload = {
            ...(existing || {}),
            name: payload.name || existing.name || getCommandMeta(commandKey).label,
            protocol: payload.protocol || existing.protocol || 'PANASONIC',
            rawData: normalizeRawData(payload.rawData || existing.rawData || []),
            signalLength: Number(payload.signalLength || existing.signalLength || normalizeRawData(payload.rawData || existing.rawData || []).length || 0),
            updatedAt: new Date().toISOString()
        };

        await db.ref(`irRemoteLibrary/${getLabKey(labName)}/${commandKey}`).set(nextPayload);
        return nextPayload;
    }

    function getLibraryForLab(libraryData, labName) {
        const labKey = getLabKey(labName);
        const source = libraryData?.[labKey] || {};
        return COMMAND_OPTIONS.reduce((acc, commandMeta) => {
            const item = source[commandMeta.key];
            acc[commandMeta.key] = item || null;
            return acc;
        }, {});
    }

    function mapLibraryToRows(libraryData) {
        const rows = [];
        LABORATORY_OPTIONS.forEach((labName) => {
            const labKey = getLabKey(labName);
            const labData = libraryData?.[labKey] || {};
            rows.push({
                labName,
                labKey,
                signals: COMMAND_OPTIONS.map((meta) => ({
                    commandKey: meta.key,
                    commandLabel: meta.label,
                    group: meta.group,
                    data: labData[meta.key] || null
                }))
            });
        });
        return rows;
    }

    function getSignalText(entry) {
        if (!entry) return `uint16_t rawData[] = {\n    0\n};`;
        return makeSignalString(entry);
    }

    function signalExists(libraryData, labName, commandKey) {
        const labKey = getLabKey(labName);
        return !!(libraryData?.[labKey]?.[commandKey]);
    }

    function parseSignalFromFirebase(snapshotValue) {
        if (!snapshotValue) return null;
        return {
            ...snapshotValue,
            protocol: snapshotValue.protocol || 'PANASONIC',
            rawData: normalizeRawData(snapshotValue.rawData || []),
            signalLength: Number(snapshotValue.signalLength || (snapshotValue.rawData || []).length || 0),
            name: snapshotValue.name || 'IR Signal'
        };
    }

    window.IRLibraryManager = {
        LABORATORY_OPTIONS,
        COMMAND_OPTIONS,
        getDb,
        getAuth,
        isAuthorizedIRUser,
        fetchIRLibrary,
        saveIRSignal,
        updateIRSignal,
        deleteIRSignal,
        getLibraryForLab,
        mapLibraryToRows,
        signalExists,
        getSignalText,
        parseSignalFromFirebase,
        getLabKey,
        getCommandMeta,
        normalizeRawData,
        buildLibraryEntry,
        makeSignalString,
        config: CONFIG
    };
})();
