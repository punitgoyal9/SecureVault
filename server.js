import http from 'http';
import fs from 'fs';
import path from 'path';
import { execFile, exec } from 'child_process';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const PORT = 3000;

const LOG_DIR = 'C:\\SecureVaultData';
const BACKUP_DIR = path.join(LOG_DIR, 'backup');
const LOG_FILE = path.join(LOG_DIR, 'log.txt');

if (!fs.existsSync(LOG_DIR)) fs.mkdirSync(LOG_DIR, { recursive: true });
if (!fs.existsSync(BACKUP_DIR)) fs.mkdirSync(BACKUP_DIR, { recursive: true });

function writeLog(action, inPath, outPath, status, extra = '') {
    const timestamp = new Date().toLocaleString();
    const logEntry = `[${timestamp}] | ACTION: ${action} | IN: "${inPath}" | OUT: "${outPath}" | STATUS: ${status} ${extra ? '| ' + extra : ''}\n`;
    fs.appendFileSync(LOG_FILE, logEntry, 'utf8');
}

// Helper function to find original extension & backup filename from log.txt
function findLogDetails(vaultPath) {
    try {
        if (!fs.existsSync(LOG_FILE)) return { ext: '', backupFile: '' };
        const data = fs.readFileSync(LOG_FILE, 'utf8');
        const lines = data.split('\n');
        
        for (let i = lines.length - 1; i >= 0; i--) {
            if (lines[i].includes('ACTION: ENCRYPT') && lines[i].includes(vaultPath)) {
                let extMatch = lines[i].match(/ORIGINAL_EXT:\s*([^\s|]+)/);
                let backupMatch = lines[i].match(/BACKUP_FILE:\s*([^\s|]+)/);
                
                return {
                    ext: extMatch ? extMatch[1] : '',
                    backupFile: backupMatch ? backupMatch[1] : ''
                };
            }
        }
    } catch (err) {
        console.error("Error reading log details:", err);
    }
    return { ext: '', backupFile: '' };
}

const server = http.createServer((req, res) => {
    if (req.url === '/api/browse' && req.method === 'GET') {
        const psCommand = `Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.ShowDialog() | Out-Null; $f.FileName`;
        exec(`powershell -Command "${psCommand}"`, (error, stdout) => {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            if (error) {
                res.end(JSON.stringify({ success: false, filePath: '' }));
            } else {
                res.end(JSON.stringify({ success: true, filePath: stdout.trim() }));
            }
        });
        return;
    }

    if (req.url.startsWith('/api/vault') && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => { body += chunk; });
        req.on('end', () => {
            try {
                let { mode, inPath, outPath, password } = JSON.parse(body);

                if (!inPath || !outPath || !password) {
                    res.writeHead(400, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify({ success: false, message: 'All fields are required!' }));
                    return;
                }

                const exePath = path.join(__dirname, 'secure_core.exe');
                let originalExt = '';
                let backupFileName = '';

                if (mode === 'enc') {
                    if (!outPath.toLowerCase().endsWith('.vault')) {
                        outPath += '.vault';
                    }
                    originalExt = path.extname(inPath);

                    // Save backup in C:\SecureVaultData\backup
                    try {
                        backupFileName = `${Date.now()}_${path.basename(inPath)}`;
                        const backupDest = path.join(BACKUP_DIR, backupFileName);
                        fs.copyFileSync(inPath, backupDest);
                    } catch (backupErr) {
                        console.error("Backup failed:", backupErr);
                    }
                } else if (mode === 'dec') {
                    const logDetails = findLogDetails(inPath);
                    originalExt = logDetails.ext;
                    backupFileName = logDetails.backupFile;

                    if (originalExt && !outPath.toLowerCase().endsWith(originalExt.toLowerCase())) {
                        outPath = outPath + originalExt;
                    }
                }

                // Execute C binary core smoothly without authentication blocking
                execFile(exePath, [mode, inPath, outPath, password], (error, stdout, stderr) => {
                    res.writeHead(200, { 'Content-Type': 'application/json' });
                    if (error) {
                        writeLog(mode === 'enc' ? 'ENCRYPT' : 'DECRYPT', inPath, outPath, 'FAILED', stderr || error.message);
                        res.end(JSON.stringify({ success: false, message: stderr || error.message }));
                    } else {
                        if (mode === 'enc') {
                            const extraInfo = `ORIGINAL_EXT: ${originalExt} | BACKUP_FILE: ${backupFileName}`;
                            writeLog('ENCRYPT', inPath, outPath, 'SUCCESS', extraInfo);
                        } else {
                            // Remove backup file from C Drive on successful decryption
                            if (backupFileName) {
                                const backupFilePath = path.join(BACKUP_DIR, backupFileName);
                                if (fs.existsSync(backupFilePath)) {
                                    try {
                                        fs.unlinkSync(backupFilePath);
                                    } catch (delErr) {
                                        console.error("Failed to delete backup file:", delErr);
                                    }
                                }
                            }
                            writeLog('DECRYPT', inPath, outPath, 'SUCCESS', `REMOVED_BACKUP: ${backupFileName}`);
                        }

                        res.end(JSON.stringify({ success: true, message: `${stdout.trim()} (Path: ${outPath})` }));
                    }
                });

            } catch (err) {
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, message: 'Invalid JSON payload' }));
            }
        });
        return;
    }

    let filePath = path.join(__dirname, 'public', req.url === '/' ? 'index.html' : req.url);
    fs.readFile(filePath, (err, data) => {
        if (err) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('404 Not Found');
        } else {
            res.writeHead(200);
            res.end(data);
        }
    });
});

server.listen(PORT, () => {
    console.log(`SecureVault Server running live at http://localhost:${PORT}`);
});