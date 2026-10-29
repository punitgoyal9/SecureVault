# SecureVault Project Report

## 1. Project Overview

SecureVault is a Windows-oriented web application for encrypting and decrypting files. It combines a browser-based interface, a Node.js server, and a compiled C program that performs the file transformation.

The user selects an input file, enters a password, and chooses encryption or decryption. The interface generates a suggested output path, and the server passes the request to the C executable. The project's main components are `public/index.html`, `server.js`, `secure_ui.c`, and `secure_core.exe`.

## 2. Project Structure and Components

### `package.json`

The `package.json` file identifies the project as `secure-vault-app`, version `1.0.0`, and configures it to use JavaScript ES modules. It provides one script:

- `npm run dev` — starts `server.js` with Node.js watch mode.

No dependencies, test scripts, or build scripts are listed in this file.

### `public/index.html`

The `public/index.html` file provides the user interface. It includes:

- An input file path field and a **Browse** button.
- An output path field, automatically suggested in the same folder as the input file.
- A password field.
- **Encrypt File** and **Decrypt File** buttons.
- A status area for displaying progress and server responses.

The page requests a Windows file-picker dialog through the server. It suggests adding `.vault` for encryption and removing `.vault` from the suggested path for decryption. The output path remains editable. Tailwind CSS and the Inter font are loaded from external services.

### `server.js`

The `server.js` file implements an HTTP server on port `3000`. It serves the web page and handles two API routes:

- `GET /api/browse` — invokes PowerShell to open a Windows file-selection dialog.
- `POST /api/vault` — validates the request fields, prepares paths and backup information, then runs `secure_core.exe` with the selected mode, paths, and password.

The server creates `C:\SecureVaultData` and its `backup` subfolder if they do not exist. It appends operation records to `C:\SecureVaultData\log.txt`. During encryption, it attempts to copy the input file into the backup folder and records the original file extension and backup filename in the log. During decryption, it uses those log details to restore the original extension and attempts to remove the corresponding backup after successful completion.

### `secure_ui.c`

The `secure_ui.c` file contains the source code for the file-processing program. It accepts a mode, input path, output path, and password. Its encryption and decryption routines transform file bytes using XOR operations involving password characters and a fixed position-based value. A transformed `SECUREVALT1` header is written to the encrypted file and checked during decryption.

The source also attempts to delete the original file after encryption and the `.vault` input after decryption. These are ordinary file-deletion operations, not a guarantee that the data has been securely erased from storage.

### `secure_core.exe`

The project includes `secure_core.exe`, which the server invokes to process files. The executable is a binary, so its contents were not reviewed as source; its role is inferred from how the server calls it and the available C source. The executable should be kept consistent with `secure_ui.c` when the source changes.

## 3. Selected Code Snippets

The following short excerpts illustrate the main responsibilities of the interface, server, and C file-processing program. They are selected examples rather than the complete source code.

### Frontend: Output Path Generation and Server Request

From `public/index.html`, the interface suggests an output path based on whether the selected input is a `.vault` file:

```javascript
function generateOutputPath() {
    const inPath = document.getElementById('inPath').value.trim();
    const outPathInput = document.getElementById('outPath');

    if (!inPath) {
        outPathInput.value = '';
        return;
    }

    if (inPath.toLowerCase().endsWith('.vault')) {
        outPathInput.value = inPath.slice(0, -6);
    } else {
        outPathInput.value = inPath + '.vault';
    }
}
```

When the user starts an operation, the interface sends the selected mode, paths, and password to the server:

```javascript
const response = await fetch('/api/vault', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ mode, inPath, outPath, password })
});
const result = await response.json();
```

### Backend: Validation and Native Program Invocation

From `server.js`, the server rejects requests missing required values and invokes the C program with the selected operation details:

```javascript
if (!inPath || !outPath || !password) {
    res.writeHead(400, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({
        success: false,
        message: 'All fields are required!'
    }));
    return;
}

execFile(exePath, [mode, inPath, outPath, password], (error, stdout, stderr) => {
    if (error) {
        res.end(JSON.stringify({
            success: false,
            message: stderr || error.message
        }));
        return;
    }

    res.end(JSON.stringify({
        success: true,
        message: `${stdout.trim()} (Path: ${outPath})`
    }));
});
```

### C Core: Byte Transformation

From `secure_ui.c`, the program processes file data in blocks and transforms each byte using the repeating password and a position-based value:

```c
while ((bytes_read = fread(ibuf, 1, BUF_SIZE, fin)) > 0) {
    for (size_t i = 0; i < bytes_read; i++) {
        ibuf[i] = ibuf[i]
            ^ pwd[(total_index + i) % pwd_len]
            ^ ((i + 7) & 0xFF);
    }
    fwrite(ibuf, 1, bytes_read, fout);
    total_index += bytes_read;
}
```

The file also transforms a fixed header when encrypting and checks the corresponding header during decryption. As noted in the security considerations, this XOR-based method is illustrative and should not be used as production-grade encryption.

## 4. Typical Operation Flow

### Encryption

1. The user selects a file in the interface.
2. The page suggests an output path by adding `.vault`.
3. The user enters a password and selects **Encrypt File**.
4. The browser sends the mode, paths, and password to `/api/vault`.
5. The server attempts to back up the input file and runs `secure_core.exe`.
6. The C program writes the transformed file and attempts to remove the original.
7. The server records the result and displays a response in the page.

### Decryption

1. The user selects a `.vault` file.
2. The page suggests an output path without the `.vault` suffix.
3. The server searches its log for encryption details associated with the input path.
4. If it finds the original extension, it appends it to the output path where needed.
5. The server runs `secure_core.exe` in decryption mode.
6. The C program checks the header, writes the output, and attempts to remove the vault file.
7. The server attempts to remove the associated backup and records the result.

## 5. Data Handling and Security Considerations

- **The transformation is not modern cryptographic encryption.** The C source uses a repeating-password XOR scheme without a cryptographic key-derivation function, random salt or nonce, or authenticated integrity check. It should not be relied on to protect sensitive files.
- **The backup is a regular file copy.** The server attempts to place a copy of the original in `C:\SecureVaultData\backup`; the source does not encrypt that backup.
- **File deletion is consequential.** The C program attempts to delete the original after encryption and the encrypted input after decryption. A failed operation may leave the file in place, and deletion does not ensure secure erasure.
- **The project is Windows-specific.** The server uses a Windows path and PowerShell, and the executable is a Windows binary.
- **The server has no visible authentication layer.** The UI sends the password to the server as part of the request, and the server passes it to the executable as a process argument. The reviewed code does not show user authentication or encrypted transport configuration.
- **Error reporting may be misleading.** Some C errors are printed as messages without making the program exit with a failure status. Since the server primarily checks whether execution itself failed, it may treat some unsuccessful file operations as successful.
- **The decryption workflow depends on server logs.** Restoring the original extension and locating the backup rely on a matching encryption record in `log.txt`. If that record is unavailable, the server may not recover those details.

## 6. Summary

SecureVault demonstrates a web-to-native workflow: an HTML interface sends requests to a Node.js server, which invokes a C program to process files. It includes automatic output-path suggestions, a Windows file picker, logging, and an attempted backup-and-restore workflow. However, the custom XOR transformation, unencrypted backup copy, destructive deletion behavior, and limited error signaling mean the current implementation should be treated as a project demonstration rather than a dependable solution for protecting important or sensitive data.
