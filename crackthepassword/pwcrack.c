#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <assert.h>

const int SHA_LENGTH = 32;
const int testing = 1;

uint8_t hex_to_byte(unsigned char h1, unsigned char h2) {
    int most_sig = 0;   // 5   
    int least_sig = 0;  // 10
    int base = 16;
    if (h1 >= '0' && h1 <= '9') {
        most_sig = h1 - '0';           // 5                              
    }
    if (h1 >= 'A' && h1 <= 'F') {   
        most_sig = h1 - 'A' + 10;      // 10 
    }
    if (h1 >= 'a' && h1 <= 'f') {
        most_sig = h1 - 'a' + 10; 
    }  
    if (h2 >= '0' && h2 <= '9') {
        least_sig = h2 - '0';           // 5                              
    }
    if (h2 >= 'A' && h2 <= 'F') {   
        least_sig = h2 - 'A' + 10;      // 10 
    }
    if (h2 >= 'a' && h2 <= 'f') {
        least_sig = h2 - 'a' + 10; 
    }       
    return (most_sig * base) + least_sig;
}

void test_hex_to_byte() {
    assert(hex_to_byte('c', '8') == 200);
    assert(hex_to_byte('0', '3') == 3);
    assert(hex_to_byte('0', 'a') == 10);
    assert(hex_to_byte('1', '0') == 16);
}

void hexstr_to_hash(unsigned char hexstr[], uint8_t hash[SHA_LENGTH]) {
    int len = strlen((char *)hexstr);
    int i;
    for (i = 0; i < len / 2; i++) {    
         hash[i] = hex_to_byte(hexstr[2 * i], hexstr[2 * i + 1]);
    }
}

void test_hexstr_to_hash() {
    char hexstr[64] = "a2c3b02cb22af83d6d1ead1d4e18d916599be7c2ef2f017169327df1f7c844fd";
    uint8_t hash[SHA_LENGTH];
    hexstr_to_hash((unsigned char *)hexstr, hash);
   // printf("Test hexstr to hash: %s\n", hexstr);
   //int i;
   // for (i = 0; i < SHA_LENGTH; i++) {
   //    printf("0b%hhx\t", hash[i]);
   //}
   //printf("\n");
    assert(hash[0] == (uint8_t)0xa2);
    assert(hash[31] == (uint8_t)0xfd);
}

int8_t check_password(char password[], uint8_t given_hash[SHA_LENGTH]) {
    uint8_t target_hash[SHA_LENGTH];
    SHA256((unsigned char *)password, strlen(password), target_hash); // result stored in target_hash
    int i;
    for (i = 0; i < SHA_LENGTH; i++) {
        if (target_hash[i] != given_hash[i]) {
            return 0;  
        }
    }
    return 1; 
}

void test_check_password() {
    char hash_as_hexstr[] = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"; // SHA256 hash for "password"
    uint8_t given_hash[SHA_LENGTH];
    hexstr_to_hash((unsigned char *)hash_as_hexstr, given_hash);
    assert(check_password("password", given_hash) == 1);
    assert(check_password("wrongpass", given_hash) == 0);
}


int8_t crack_password(char password[], unsigned char given_hash[]) {
    if (check_password(password, given_hash)) {
        return 1; // password cracked
    }
    else{
        int i;
        for (i = 0; password[i] != '\0'; i++) {
            char original_char = password[i];  
            if (original_char >= 'a' && original_char <= 'z') {
                 password[i] -= 32;  
                 if (check_password(password, given_hash)) {
                   // printf("Found password variation by changing '%c' to '%c': SHA256(%s)\n", original_char, password[i], password);
                    return 1; 
                }
                password[i] = original_char;  
            }      
            else if (original_char >= 'A' && original_char <= 'Z') {
                password[i] += 32;  
                if (check_password(password, given_hash)) {
                 //   printf("Found password variation by changing '%c' to '%c': SHA256(%s)\n", original_char, password[i], password);
                    return 1; 
                }
                password[i] = original_char;  
            }
        }
    }
    return 0; 
}

void test_crack_password() {
    char password[] = "paSsword";
    char hash_as_hexstr[] = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"; // SHA256 hash of "password"
    uint8_t given_hash[SHA_LENGTH];
    hexstr_to_hash((unsigned char *)hash_as_hexstr, given_hash);
    int8_t match = crack_password(password, given_hash);
    assert(match == 1);
    assert(password[2] == 's'); 
}

int main(int argc, char** argv) {

    if (testing) {
       test_hex_to_byte();
       test_hexstr_to_hash();
       test_check_password();
       test_crack_password();
       // return 0;
    }
    uint8_t hash[SHA_LENGTH];
    char hash_as_hexstr[65];  
    strcpy(hash_as_hexstr, argv[1]);   
    hexstr_to_hash((unsigned char *)hash_as_hexstr, hash);   // convert hex 5e88... to bytes 
    char password[100]; 
    while (fgets(password, sizeof(password), stdin)) {
        password[strcspn(password, "\n")] = '\0';        
        if (check_password(password, hash)) {
            printf("Found password: SHA256(%s) = %s\n", password, hash_as_hexstr);
            return 0;   
        }  
        if(crack_password(password,hash)){
            printf("Found password: SHA256(%s) = %s\n", password, hash_as_hexstr);
            return 0; 
        }
    }
    printf("<Press Ctrl-D for end of input>\n");
    printf("Did not find a matching password\n");
    return 0;
}
