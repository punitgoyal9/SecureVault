#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 4096
#define MAGIC_HEADER "SECUREVALT1"

// Encryption Logic with Auto-Deletion of Original File
void encrypt_file(const char *in_path, const char *out_path, const char *pwd) {
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        printf("ERROR: Input file does not exist!\n");
        return;
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        printf("ERROR: Output File I/O Error!\n");
        fclose(fin);
        return;
    }

    int pwd_len = strlen(pwd);
    char header_buffer[11];
    memcpy(header_buffer, MAGIC_HEADER, 11);
    for (int i = 0; i < 11; i++) {
        header_buffer[i] = header_buffer[i] ^ pwd[i % pwd_len] ^ ((i + 7) & 0xFF);
    }
    fwrite(header_buffer, 1, 11, fout);

    unsigned char ibuf[BUF_SIZE];
    size_t bytes_read;
    size_t total_index = 0;

    while ((bytes_read = fread(ibuf, 1, BUF_SIZE, fin)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            ibuf[i] = ibuf[i] ^ pwd[(total_index + i) % pwd_len] ^ ((i + 7) & 0xFF);
        }
        fwrite(ibuf, 1, bytes_read, fout);
        total_index += bytes_read;
    }

    fclose(fin);
    fclose(fout);

    // SECURITY FEATURE: Delete original file after successful encryption
    if (remove(in_path) != 0) {
        printf("WARNING: Encryption successful, but failed to delete original file.\n");
    } else {
        printf("SUCCESS: Encryption Successful! Original file securely removed.\n");
    }
}

// Decryption Logic with Auto-Deletion of Encrypted Vault File
void decrypt_file(const char *in_path, const char *out_path, const char *pwd) {
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        printf("ERROR: Encrypted vault file does not exist!\n");
        return;
    }

    char header_buffer[11];
    if (fread(header_buffer, 1, 11, fin) != 11) {
        printf("ERROR: Invalid or corrupted vault file!\n");
        fclose(fin);
        return;
    }

    int pwd_len = strlen(pwd);
    for (int i = 0; i < 11; i++) {
        header_buffer[i] = header_buffer[i] ^ pwd[i % pwd_len] ^ ((i + 7) & 0xFF);
    }

    if (memcmp(header_buffer, MAGIC_HEADER, 11) != 0) {
        printf("ERROR: Access Denied: Wrong password!\n");
        fclose(fin);
        return;
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        printf("ERROR: Output File Error!\n");
        fclose(fin);
        return;
    }

    unsigned char ibuf[BUF_SIZE];
    size_t bytes_read;
    size_t total_index = 0;

    while ((bytes_read = fread(ibuf, 1, BUF_SIZE, fin)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            ibuf[i] = ibuf[i] ^ pwd[(total_index + i) % pwd_len] ^ ((i + 7) & 0xFF);
        }
        fwrite(ibuf, 1, bytes_read, fout);
        total_index += bytes_read;
    }

    fclose(fin);
    fclose(fout);

    // SECURITY FEATURE: Delete encrypted .vault file after successful decryption
    if (remove(in_path) != 0) {
        printf("WARNING: Decryption successful, but failed to remove vault file.\n");
    } else {
        printf("SUCCESS: Decryption Successful! Vault file securely removed.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <enc/dec> <input> <output> <password>\n", argv[0]);
        return 1;
    }

    // argv[4] is the vault app password, but let's make sure arguments match
    const char *mode = argv[1];
    const char *in = argv[2];
    const char *out = argv[3];
    const char *pwd = (argc >= 5) ? argv[4] : "";

    if (strcmp(mode, "enc") == 0) {
        encrypt_file(in, out, pwd);
    } else if (strcmp(mode, "dec") == 0) {
        decrypt_file(in, out, pwd);
    } else {
        printf("ERROR: Unknown mode.\n");
    }
    return 0;
}




// #include <stdio.h>
// #include <string.h>

// #define BUF_SIZE 4096
// #define MAGIC_HEADER "SECUREVALT1"

// void encrypt_file(const char *in_path, const char *out_path, const char *pwd) {
//     FILE *fin = fopen(in_path, "rb");
//     if (!fin) { printf("ERROR: Input file does not exist!\n"); return; }

//     FILE *fout = fopen(out_path, "wb");
//     if (!fout) { printf("ERROR: Output File I/O Error!\n"); fclose(fin); return; }

//     int pwd_len = strlen(pwd);
//     char header_buffer[11];
//     memcpy(header_buffer, MAGIC_HEADER, 11);
//     for (int i = 0; i < 11; i++) {
//         header_buffer[i] = header_buffer[i] ^ pwd[i % pwd_len] ^ ((i + 7) & 0xFF);
//     }
//     fwrite(header_buffer, 1, 11, fout);

//     unsigned char ibuf[BUF_SIZE];
//     size_t bytes_read;
//     size_t total_index = 0;

//     while ((bytes_read = fread(ibuf, 1, BUF_SIZE, fin)) > 0) {
//         for (size_t i = 0; i < bytes_read; i++) {
//             ibuf[i] = ibuf[i] ^ pwd[(total_index + i) % pwd_len] ^ ((i + 7) & 0xFF);
//         }
//         fwrite(ibuf, 1, bytes_read, fout);
//         total_index += bytes_read;
//     }

//     fclose(fin);
//     fclose(fout);
//     printf("SUCCESS: Encryption Successful!\n");
// }

// void decrypt_file(const char *in_path, const char *out_path, const char *pwd) {
//     FILE *fin = fopen(in_path, "rb");
//     if (!fin) { printf("ERROR: Input file does not exist!\n"); return; }

//     char header_buffer[11];
//     if (fread(header_buffer, 1, 11, fin) != 11) {
//         printf("ERROR: Invalid or corrupted vault file!\n");
//         fclose(fin);
//         return;
//     }

//     int pwd_len = strlen(pwd);
//     for (int i = 0; i < 11; i++) {
//         header_buffer[i] = header_buffer[i] ^ pwd[i % pwd_len] ^ ((i + 7) & 0xFF);
//     }

//     if (memcmp(header_buffer, MAGIC_HEADER, 11) != 0) {
//         printf("ERROR: Access Denied: Wrong password!\n");
//         fclose(fin);
//         return;
//     }

//     FILE *fout = fopen(out_path, "wb");
//     if (!fout) { printf("ERROR: Output File Error!\n"); fclose(fin); return; }

//     unsigned char ibuf[BUF_SIZE];
//     size_t bytes_read;
//     size_t total_index = 0;

//     while ((bytes_read = fread(ibuf, 1, BUF_SIZE, fin)) > 0) {
//         for (size_t i = 0; i < bytes_read; i++) {
//             ibuf[i] = ibuf[i] ^ pwd[(total_index + i) % pwd_len] ^ ((i + 7) & 0xFF);
//         }
//         fwrite(ibuf, 1, bytes_read, fout);
//         total_index += bytes_read;
//     }

//     fclose(fin);
//     fclose(fout);
//     printf("SUCCESS: Decryption Successful!\n");
// }

// int main(int argc, char *argv[]) {
//     if (argc < 5) {
//         printf("Usage: %s <enc/dec> <input> <output> <password>\n", argv[0]);
//         return 1;
//     }

//     if (strcmp(argv[1], "enc") == 0) {
//         encrypt_file(argv[2], argv[3], argv[4]);
//     } else if (strcmp(argv[1], "dec") == 0) {
//         decrypt_file(argv[2], argv[3], argv[4]);
//     } else {
//         printf("ERROR: Unknown mode. Use 'enc' or 'dec'.\n");
//     }
//     return 0;
// }