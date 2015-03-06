#include <gtk/gtk.h>
#include <jansson.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "gonepass.h"



struct master_key level3_key, level5_key;
int credentials_loaded = 0;

int decode_base64(const char * input, ssize_t input_len, uint8_t ** output) {
    BIO * b64, *bmem;

    if(input_len < 0)
        input_len = strlen(input);

    *output = (uint8_t*)calloc(1, input_len);
    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new_mem_buf((void*)input, input_len);
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
    int size = BIO_read(bmem, *output, input_len);
    BIO_free_all(bmem);
    return size;
}

void encode_base64(uint8_t * input, ssize_t input_len, BUF_MEM ** output) {
    BIO * b64, *bmem;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, input_len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, bptr);
    BIO_set_close(bmem, BIO_NOCLOSE);
    BIO_free_all(bmem);
}

void openssl_key(uint8_t * password, size_t pwlen, uint8_t * salt, size_t saltlen,
        uint8_t * keyout, uint8_t * ivout) {
    const int rounds = 2;
    int i;
    MD5_CTX ctx;

    // Initial digest is password + salt
    MD5_Init(&ctx);
    MD5_Update(&ctx, password, pwlen);
    MD5_Update(&ctx, salt, saltlen);
    MD5_Final(keyout, &ctx);

    // Subsequent digests are last digest + password + salt
    MD5_Init(&ctx);
    MD5_Update(&ctx, keyout, 16);
    MD5_Update(&ctx, password, pwlen);
    MD5_Update(&ctx, salt, saltlen);
    MD5_Final(ivout, &ctx);
}

void decrypt_master_key(json_t * input, const char * master_pwd, struct master_key * out) {
    const char * encoded_key_data, *validation, *level, *identifier;
    int iterations, outlen;
    EVP_CIPHER_CTX ctx;

    json_unpack(input, "{ s:s s:s s:i s:s s:s }",
        "data", &encoded_key_data,
        "identifier", &identifier,
        "iterations", &iterations,
        "level", &level,
        "validation", &validation
    );

    uint8_t * key_data, *real_key_start;
    int key_data_len = decode_base64(encoded_key_data, -1, &key_data);
    uint8_t salt[8], master_key[32];

    // If the decoded key starts with "Salted__"
    // Then key_data[8:16] is the salt for the master password
    memset(salt, 0, sizeof(salt));
    if(strncmp(key_data, "Salted__", strlen("Salted__")) == 0) {
        memcpy(salt, key_data + 8, 8);
        real_key_start = (uint8_t*)key_data + 16;
        key_data_len -= 16;
    }
    // Otherwise, the salt is a bunch'o zeros.
    else
        real_key_start = key_data;

    // Generate a 32-byte key from the master password and its salt
    int rc = PKCS5_PBKDF2_HMAC_SHA1(
        master_pwd, strlen(master_pwd), // The master password is input
        salt, sizeof(salt), // The salt is the key_data[8:16]
        iterations,
        sizeof(master_key),
        master_key
    );


    EVP_CIPHER_CTX_init(&ctx);
    // Initialize a decrypting AES key with the master key and master key + 16 as the salt
    EVP_CipherInit_ex(&ctx, EVP_aes_128_cbc(), NULL, master_key, master_key + 16, 0);
    EVP_CipherUpdate(&ctx, out->key_data, &outlen, real_key_start, key_data_len);
    out->key_len = outlen;

    if(!EVP_CipherFinal_ex(&ctx, out->key_data + outlen, &outlen)) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        fprintf(stderr, "Couldn't decrypt master key!\n");
        exit(1);
    }
    out->key_len += outlen;
    EVP_CIPHER_CTX_cleanup(&ctx);

    free(key_data);

    uint8_t * validation_data, *real_validation_data_start;
    uint8_t validation_key[16], validation_iv[16];
    uint8_t decrypted_validation_key[1056];

    int validation_data_len = decode_base64(validation, -1, &validation_data);
     // If the decoded key starts with "Salted__"
    memset(salt, 0, sizeof(salt));
    if(strncmp(validation_data, "Salted__", strlen("Salted__")) == 0) {
        memcpy(salt, validation_data + 8, 8);
        real_validation_data_start = validation_data + 16;
        validation_data_len -= 16;

        // From the salt and the decrypted key, we get a aes key/iv
        openssl_key(
            out->key_data, out->key_len,
            salt, sizeof(salt),
            validation_key, validation_iv
        );
    }
    // Otherwise, the salt is a bunch'o zeros.
    // And the key is the md5 of the decrypted master key
    else {
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, out->key_data, out->key_len);
        MD5_Final(validation_key, &md5);

        real_validation_data_start = validation_data;
    }

    EVP_CIPHER_CTX_init(&ctx);
    // We generate a decrypting AES key with the output of openssl_key above
    EVP_CipherInit_ex(&ctx, EVP_aes_128_cbc(), NULL, validation_key, validation_iv, 0);
    EVP_CipherUpdate(&ctx,
        decrypted_validation_key, &outlen,
        real_validation_data_start, validation_data_len
    );
    if(!EVP_CipherFinal_ex(&ctx, decrypted_validation_key + outlen, &outlen)) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        fprintf(stderr, "Couldn't decrypt validation key!\n");
        exit(1);
    }

    free(validation_data);

    if(memcmp(decrypted_validation_key, out->key_data, out->key_len) != 0) {
        fprintf(stderr, "Validation cleartext doesn't match key data!\n");
        exit(1);
    }

    if(strcmp(level, "SL3") == 0)
        out->level = 3;
    else if(strcmp(level, "SL5") == 0)
        out->level = 5;
    strcpy(out->id, identifier);
}

int decrypt_item(json_t * input, struct credentials_bag * bag, char ** output) {
    const char *encrypted_encoded_payload, *security_level;
    EVP_CIPHER_CTX ctx;

    json_unpack(input, "{s:s s:s}",
        "encrypted", &encrypted_encoded_payload,
        "securityLevel", &security_level
    );

    struct master_key * master_key = NULL;
    if(strcmp(security_level, "SL5") == 0)
        master_key = &bag->level5_key;
    else if(strcmp(security_level, "SL3") == 0)
        master_key = &bag->level3_key;

    uint8_t * encrypted_payload, salt[8], *real_encrypted_payload;
    uint8_t my_key[16], my_iv[16];
    int payload_size = decode_base64(encrypted_encoded_payload, -1, &encrypted_payload);
    int outlen;

    memset(salt, 0, sizeof(salt));
    if(strncmp(encrypted_payload, "Salted__", strlen("Salted__")) == 0) {
        memcpy(salt, encrypted_payload + 8, 8);
        real_encrypted_payload = encrypted_payload + 16;
        payload_size -= 16;

        // From the salt and the decrypted key, we get a aes key/iv
        openssl_key(
            master_key->key_data, master_key->key_len,
            salt, sizeof(salt),
            my_key, my_iv
        );
    }
    // Otherwise, the salt is a bunch'o zeros.
    // And the key is the md5 of the decrypted master key
    else {
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, master_key->key_data, master_key->key_len);
        MD5_Final(my_key, &md5);

        real_encrypted_payload = encrypted_payload;
    }

    int out_size = 0;
    *output = calloc(1, payload_size);

    EVP_CIPHER_CTX_init(&ctx);
    // We generate a decrypting AES key with the output of openssl_key above
    EVP_CipherInit_ex(&ctx, EVP_aes_128_cbc(), NULL, my_key, my_iv, 0);
    EVP_CipherUpdate(&ctx,
        *output, &outlen,
        real_encrypted_payload, payload_size
    );
    out_size = outlen;
    if(!EVP_CipherFinal_ex(&ctx, *output + outlen, &outlen)) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        fprintf(stderr, "Couldn't decrypt payload!\n");
        exit(1);
    }
    out_size += outlen;
    free(encrypted_payload);
    return out_size;
}

void clear_credentials(struct credentials_bag * bag) {
    memset(&bag->level3_key, 0, sizeof(bag->level3_key));
    memset(&bag->level5_key, 0, sizeof(bag->level5_key));
    credentials_loaded = 0;
}

int load_credentials(GonepassAppWindow * win, struct credentials_bag * out) {
    json_error_t errmsg;

    GonepassUnlockDialog * unlock_dialog = gonepass_unlock_dialog_new(win);
    int dlg_return = gtk_dialog_run(GTK_DIALOG(unlock_dialog));
    if(dlg_return != GTK_RESPONSE_OK) {
        fprintf(stderr, "The unlock dialog returned a mysterious exit code %d\n", dlg_return);
        return 1;
    }
    const gchar * passwd = gonepass_unlock_dialog_get_pass(unlock_dialog);
    const gchar * vault_folder = gonepass_unlock_dialog_get_vault_path(unlock_dialog);

    char vault_path[PATH_MAX];
    GSettings * settings = g_settings_new ("org.gtk.gonepass");
    snprintf(vault_path, sizeof(vault_path), "%s/data/default/encryptionKeys.js", vault_folder);
    json_t * encryption_keys_json = json_load_file(vault_path, 0, &errmsg);

    if(encryption_keys_json == NULL) {
        fprintf(stderr, "Error loading encryption keys! %s\n", errmsg.text);
        return 1;
    }

    json_t * key_array = json_object_get(encryption_keys_json, "list");
    json_t *key_value;
    size_t index;
    if(!json_is_array(key_array)) {
        fprintf(stderr, "Expecting array of keys!\n");
        return 1;
    }
    json_array_foreach(key_array, index, key_value) {
        struct master_key tmpkey;
        decrypt_master_key(key_value, passwd, &tmpkey);
        struct master_key *target_key;
        switch(tmpkey.level) {
            case 3:
                target_key = &out->level3_key;
                break;
            case 5:
                target_key = &out->level5_key;
                break;
            default:
                fprintf(stderr, "Unknown key level %d\n", tmpkey.level);
                exit(1);
        }

        memcpy(target_key, &tmpkey, sizeof(tmpkey));
    }

    gtk_window_close(GTK_WINDOW(unlock_dialog));
    out->credentials_loaded = 1;
    g_settings_set_string(settings, "vault-path", vault_folder);
    strncpy(out->vault_path, vault_folder, PATH_MAX);

    return 0;
}

int main(int argc, char ** argv) {
   g_setenv("GSETTINGS_SCHEMA_DIR", ".", FALSE);
    return g_application_run(G_APPLICATION(gonepass_app_new()), argc, argv);
}
