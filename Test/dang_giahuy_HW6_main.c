/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Name: Giahuy Dang
* Student ID: 922722304
* GitHub ID: leavism
* Project: Assignment 6 – Device Driver
*
* File: dang_giahuy_HW6_main.c
*
* Description: The driver program for the Caesar Cipher device
* driver. It prompts the user for a cipher key and then the
* message to be encrypted. It will display the encrypted message
* and then decrypts it to display the decrypted message.
**************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ENCRYPT _IO('e', 0)
#define DECRYPT _IO('e', 1)
#define SETKEY _IO('e', 3)

#define MAX_CHARS 513  // 512 chars + newline
#define MAX_KEYCHARS 100

// Function to check if a string is only whitespace
int isOnlyWhitespace(const char* str)
{
	while (*str != '\0') {
		if (!(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) return 0;
		str++;
	}
	return 1;
}

int main(int argc, char* argv[])
{
	int key, fd;
	char* keystring = malloc(MAX_KEYCHARS);
	char* endPtr;
	char* text = malloc(MAX_CHARS);
	char* response = malloc(MAX_CHARS);
	char* again = malloc(MAX_CHARS);

	while (1) {
		fd = open("/dev/caesar", O_RDWR);
		if (fd < 0) {
			perror("Failed to open device driver.");
			return fd;
		}
		printf("This is a Caesar Cipher program.\n");

		printf("Enter a key for encryption (0 - 52): ");
		// Validates key value
		while (1) {
			if (fgets(keystring, MAX_KEYCHARS, stdin) == NULL) {
				perror("Something went terribly wrong. Exiting program...\n");
				free(text);
				text = NULL;

				free(response);
				response = NULL;

				free(keystring);
				keystring = NULL;

				free(again);
				again = NULL;

				return -1;
			}

			// Remove newline character if present
			keystring[strcspn(keystring, "\n")] = 0;

			if (strlen(keystring) == MAX_KEYCHARS - 1 && keystring[MAX_KEYCHARS - 2] != '\n') {
				int c;
				while ((c = getchar()) != '\n' && c != EOF)
					;
			}

			// Reset errno to 0 before strtol call
			errno = 0;
			key = (int) strtol(keystring, &endPtr, 10);

			// Check for various conversion errors
			// The first checks for:
			// strtol succeeded OR
			// keystring has a number (but may still contain non-numbers after) OR
			// keystring is exclusively numerical
			if (errno != 0 || endPtr == keystring || *endPtr != '\0') {
				printf("Invalid input. Please enter a number between 0 - 52: ");
			} else if (key < 0 || key > 52) {  // is key between 0 - 52
				printf("Key must be between 0 and 52. Please enter a valid key: ");
			} else {  // key is valid, proceed to get text
				break;
			}
		}

		ioctl(fd, SETKEY, key);

		printf("Enter your message for encryption (maximum 512 characters):\n");
		while (1) {
			// fgets up to only 513 characters (512 chars + 1 for newline).
			// Anything beyond is flushed.
			if (fgets(text, MAX_CHARS, stdin) == NULL) {  // Handles fgets error
				perror("Something went terribly wrong. Exiting program...\n");
				free(text);
				text = NULL;

				free(response);
				response = NULL;

				free(keystring);
				keystring = NULL;

				free(again);
				again = NULL;

				return -1;
			} else {
				// Remove newline character from text
				text[strcspn(text, "\n")] = 0;

				// Checks if user passed in more than 512 characters.
				// Flushes stdin for the next loop.
				if (strlen(text) == MAX_CHARS - 1 && text[MAX_CHARS - 2] != '\n') {
					int c;
					while ((c = getchar()) != '\n' && c != EOF)
						;
				}

				// Handles when message is only whitespace
				if (isOnlyWhitespace(text)) {
					printf("That was an empty message.\nEnter your message for encryption (maximum "
								 "512 characters):\n");
				} else {  // Input met all requirements, proceed to encryption
					break;
				}
			}
		}
		printf("Message length: %li", strlen(text));
		printf("\nAny characters beyond the 512 maximum has been flushed.\n");
		printf("\nMessage to be encrypted:\n%s\n", text);

		// Write to kernel
		write(fd, text, strlen(text));

		// Call to encrypt message
		ioctl(fd, ENCRYPT);

		// Read the encrypted message
		read(fd, response, strlen(text));

		printf("\nThe encrypted message:\n%s\n\n", response);

		// Call to decrypt message
		ioctl(fd, DECRYPT);

		// Read the decrypted message
		read(fd, response, strlen(text));

		printf("The decrypted message:\n%s\n\n", response);

		printf("Re-encrypting message.\n\n");

		ioctl(fd, ENCRYPT);

		printf("Closing the encryption device...\n\n");

		close(fd);
		printf("Encrypt another message? (y/n): ");

		if (fgets(again, MAX_KEYCHARS, stdin) == NULL) {
			perror("Something went terribly wrong. Exiting program...\n");

			free(text);
			text = NULL;

			free(response);
			response = NULL;

			free(keystring);
			keystring = NULL;

			free(again);
			again = NULL;

			return -1;
		}

		// Remove newline character if present
		again[strcspn(again, "\n")] = 0;

		if (again[0] != 'y') {
			// Free allocated memory before breaking the loop
			free(text);
			text = NULL;

			free(response);
			response = NULL;

			free(keystring);
			keystring = NULL;

			free(again);
			again = NULL;

			printf("\nExiting program.\n");
			return 0;
		}

		// Clears text and response for next loop iteration
		for (int i = 0; i < MAX_CHARS; i++) {
			text[i] = '\0';
			response[i] = '\0';
		}

		for (int i = 0; i < MAX_KEYCHARS; i++) { keystring[i] = '\0'; }
	}
}