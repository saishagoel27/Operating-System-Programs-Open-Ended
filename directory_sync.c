/**
 * Directory Synchronizer
 * A client-server application for synchronizing local directory changes to a remote folder
 * Windows-compatible version
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <windows.h>
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <direct.h>
 #include <io.h>
 
 // Link with Winsock library
 #pragma comment(lib, "ws2_32.lib")
 
 #define BUFFER_SIZE 1024
 #define MAX_PATH_LENGTH 256
 #define SERVER_PORT 8888
 
 // File action operations
 typedef enum {
     SYNC_CREATE,
     SYNC_MODIFY, 
     SYNC_DELETE
 } sync_operation;
 
 // File information structure
 typedef struct {
     char path[MAX_PATH_LENGTH];
     time_t last_modified;
     long size;
     int is_directory;
 } file_info;
 
 // Synchronization record
 typedef struct {
     sync_operation operation;
     file_info file;
     char data[BUFFER_SIZE];
     size_t data_size;
 } sync_record;
 
 // Function prototypes
 void scan_directory(const char *dir_path, file_info **files, int *file_count);
 int file_exists(const char *path);
 void compare_directories(file_info *old_files, int old_count, 
                         file_info *new_files, int new_count,
                         sync_record **changes, int *change_count);
 void apply_changes(sync_record *changes, int change_count, const char *target_dir);
 int send_changes_to_server(sync_record *changes, int change_count, const char *server_ip);
 void watch_directory(const char *dir_path, int interval, const char *server_ip);
 char* normalize_path(const char* path);
 
 // Client main function
 int client_main(int argc, char *argv[]) {
     if (argc < 4) {
         printf("Usage: %s client <directory_to_watch> <server_ip> [interval_seconds]\n", argv[0]);
         return 1;
     }
     
     const char *dir_path = argv[2];
     const char *server_ip = argv[3];
     int interval = (argc > 4) ? atoi(argv[4]) : 60; // Default to 60 seconds
     
     // Initialize Winsock
     WSADATA wsaData;
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
         printf("WSAStartup failed\n");
         return 1;
     }
     
     printf("Starting directory sync client\n");
     printf("Watching directory: %s\n", dir_path);
     printf("Server IP: %s\n", server_ip);
     printf("Sync interval: %d seconds\n", interval);
     
     watch_directory(dir_path, interval, server_ip);
     
     // Cleanup Winsock
     WSACleanup();
     return 0;
 }
 
 // Server main function
 int server_main(int argc, char *argv[]) {
     if (argc < 3) {
         printf("Usage: %s server <target_directory>\n", argv[0]);
         return 1;
     }
     
     // Initialize Winsock
     WSADATA wsaData;
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
         printf("WSAStartup failed\n");
         return 1;
     }
     
     const char *target_dir = argv[2];
     SOCKET server_socket, client_socket;
     struct sockaddr_in server_addr, client_addr;
     int client_len = sizeof(client_addr);
     sync_record changes[100]; // Arbitrary limit for simplicity
     int change_count = 0;
     int recv_result;
     
     // Create socket
     server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (server_socket == INVALID_SOCKET) {
         printf("Error creating socket: %d\n", WSAGetLastError());
         WSACleanup();
         return 1;
     }
     
     // Configure server address
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = INADDR_ANY;
     server_addr.sin_port = htons(SERVER_PORT);
     
     // Bind socket
     if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
         printf("Error binding socket: %d\n", WSAGetLastError());
         closesocket(server_socket);
         WSACleanup();
         return 1;
     }
     
     // Listen for connections
     if (listen(server_socket, 5) == SOCKET_ERROR) {
         printf("Error listening: %d\n", WSAGetLastError());
         closesocket(server_socket);
         WSACleanup();
         return 1;
     }
     
     printf("Directory sync server started\n");
     printf("Target directory: %s\n", target_dir);
     printf("Listening on port %d\n", SERVER_PORT);
     
     while (1) {
         // Accept client connection
         client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
         if (client_socket == INVALID_SOCKET) {
             printf("Error accepting connection: %d\n", WSAGetLastError());
             continue;
         }
         
         char client_ip[INET_ADDRSTRLEN];
         strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
         
        printf("Connection accepted from %s\n", client_ip);
         
         // Receive changes
         recv_result = recv(client_socket, (char*)&change_count, sizeof(change_count), 0);
         if (recv_result == SOCKET_ERROR || recv_result == 0) {
             printf("Error receiving change count: %d\n", WSAGetLastError());
             closesocket(client_socket);
             continue;
         }
         
         printf("Receiving %d changes\n", change_count);
         
         for (int i = 0; i < change_count; i++) {
             recv_result = recv(client_socket, (char*)&changes[i], sizeof(sync_record), 0);
             if (recv_result == SOCKET_ERROR || recv_result == 0) {
                 printf("Error receiving change record: %d\n", WSAGetLastError());
                 break;
             }
             
             // Receive file data if needed
             if (changes[i].data_size > 0) {
                 recv_result = recv(client_socket, changes[i].data, changes[i].data_size, 0);
                 if (recv_result == SOCKET_ERROR || recv_result == 0) {
                     printf("Error receiving file data: %d\n", WSAGetLastError());
                     break;
                 }
             }
         }
         
         // Apply changes to target directory
         apply_changes(changes, change_count, target_dir);
         
         closesocket(client_socket);
     }
     
     closesocket(server_socket);
     WSACleanup();
     return 0;
 }
 
 // Convert forward slashes to backslashes for Windows paths
 char* normalize_path(const char* path) {
     char* normalized = _strdup(path);
     char* p = normalized;
     
     while (*p) {
         if (*p == '/') *p = '\\';
         p++;
     }
     
     return normalized;
 }
 
 // Scan a directory and collect file information
 void scan_directory(const char *dir_path, file_info **files, int *file_count) {
     WIN32_FIND_DATA find_data;
     HANDLE find_handle;
     char search_path[MAX_PATH_LENGTH];
     char full_path[MAX_PATH_LENGTH];
     
     *file_count = 0;
     *files = NULL;
     
     // Normalize the path
     char* normalized_dir = normalize_path(dir_path);
     
     // Prepare the search path
     snprintf(search_path, MAX_PATH_LENGTH, "%s\\*", normalized_dir);
     
     // First count the files
     find_handle = FindFirstFile(search_path, &find_data);
     if (find_handle == INVALID_HANDLE_VALUE) {
         printf("Error opening directory: %lu\n", GetLastError());
         free(normalized_dir);
         return;
     }
     
     do {
         if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
             continue;
         
         (*file_count)++;
     } while (FindNextFile(find_handle, &find_data));
     
     FindClose(find_handle);
     
     // Allocate memory for file info
     *files = (file_info *)malloc(*file_count * sizeof(file_info));
     if (!*files) {
         printf("Memory allocation failed\n");
         free(normalized_dir);
         *file_count = 0;
         return;
     }
     
     // Collect file information
     find_handle = FindFirstFile(search_path, &find_data);
     if (find_handle == INVALID_HANDLE_VALUE) {
         printf("Error reopening directory: %lu\n", GetLastError());
         free(*files);
         *files = NULL;
         *file_count = 0;
         free(normalized_dir);
         return;
     }
     
     int i = 0;
     do {
         if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
             continue;
         
         if (i >= *file_count) break; // Safety check
         
         snprintf(full_path, MAX_PATH_LENGTH, "%s\\%s", normalized_dir, find_data.cFileName);
         
         strncpy((*files)[i].path, full_path, MAX_PATH_LENGTH);
         
         // Convert Windows file time to time_t
         FILETIME ft = find_data.ftLastWriteTime;
         ULARGE_INTEGER uli;
         uli.LowPart = ft.dwLowDateTime;
         uli.HighPart = ft.dwHighDateTime;
         // Convert to seconds and adjust epoch from 1601 to 1970
         (*files)[i].last_modified = (uli.QuadPart / 10000000ULL - 11644473600ULL);
         
         // Get file size
         if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
             (*files)[i].size = 0;
             (*files)[i].is_directory = 1;
         } else {
             ULARGE_INTEGER file_size;
             file_size.LowPart = find_data.nFileSizeLow;
             file_size.HighPart = find_data.nFileSizeHigh;
             (*files)[i].size = (long)file_size.QuadPart;
             (*files)[i].is_directory = 0;
         }
         
         i++;
     } while (FindNextFile(find_handle, &find_data) && i < *file_count);
     
     FindClose(find_handle);
     free(normalized_dir);
     
     // Update file count if some files were skipped
     *file_count = i;
 }
 
 // Compare two directory scans to detect changes
 void compare_directories(file_info *old_files, int old_count, 
                          file_info *new_files, int new_count,
                          sync_record **changes, int *change_count) {
     int max_changes = old_count + new_count; // Worst case all files changed
     *changes = (sync_record *)malloc(max_changes * sizeof(sync_record));
     *change_count = 0;
     
     if (!*changes) {
         printf("Memory allocation failed\n");
         return;
     }
     
     // Detect new and modified files
     for (int i = 0; i < new_count; i++) {
         int found = 0;
         for (int j = 0; j < old_count; j++) {
             if (_stricmp(new_files[i].path, old_files[j].path) == 0) {
                 found = 1;
                 // Check if file was modified
                 if (new_files[i].last_modified > old_files[j].last_modified ||
                     new_files[i].size != old_files[j].size) {
                     (*changes)[*change_count].operation = SYNC_MODIFY;
                     (*changes)[*change_count].file = new_files[i];
                     (*change_count)++;
                 }
                 break;
             }
         }
         
         // New file
         if (!found) {
             (*changes)[*change_count].operation = SYNC_CREATE;
             (*changes)[*change_count].file = new_files[i];
             (*change_count)++;
         }
     }
     
     // Detect deleted files
     for (int i = 0; i < old_count; i++) {
         int found = 0;
         for (int j = 0; j < new_count; j++) {
             if (_stricmp(old_files[i].path, new_files[j].path) == 0) {
                 found = 1;
                 break;
             }
         }
         
         // Deleted file
         if (!found) {
             (*changes)[*change_count].operation = SYNC_DELETE;
             (*changes)[*change_count].file = old_files[i];
             (*change_count)++;
         }
     }
 }
 
 // Apply changes to a target directory
 void apply_changes(sync_record *changes, int change_count, const char *target_dir) {
     char target_path[MAX_PATH_LENGTH];
     char source_path[MAX_PATH_LENGTH];
     char* normalized_target = normalize_path(target_dir);
     
     for (int i = 0; i < change_count; i++) {
         // Create target path - need to determine relative path
         char* normalized_source = normalize_path(changes[i].file.path);
         
         // Find the base directory in the source path to create relative path
         const char* relative_path = strstr(normalized_source, "\\");
         if (relative_path) {
             // Skip the first backslash
             relative_path++;
             
             // Find the next directory separator
             while (*relative_path && *relative_path != '\\')
                 relative_path++;
             
             if (*relative_path) {
                 // Skip this separator too to get to the relative path
                 relative_path++;
                 snprintf(target_path, MAX_PATH_LENGTH, "%s\\%s", normalized_target, relative_path);
             } else {
                 // Just use the filename if we can't find a proper relative path
                 char* filename = strrchr(normalized_source, '\\');
                 if (filename) {
                     filename++; // Skip the backslash
                     snprintf(target_path, MAX_PATH_LENGTH, "%s\\%s", normalized_target, filename);
                 } else {
                     // Fallback to just using the source path directly
                     strncpy(target_path, normalized_source, MAX_PATH_LENGTH);
                 }
             }
         } else {
             // Fallback if we can't parse the path
             strncpy(target_path, normalized_source, MAX_PATH_LENGTH);
         }
         
         printf("Processing %s -> %s\n", normalized_source, target_path);
         
         // Make sure the directory exists
         char* last_slash = strrchr(target_path, '\\');
         if (last_slash) {
             *last_slash = '\0'; // Temporarily terminate the string at the directory
             // Create all directories in the path
             char* path_segment = target_path;
             while ((path_segment = strchr(path_segment, '\\')) != NULL) {
                 *path_segment = '\0'; // Temporarily terminate
                 _mkdir(target_path); // Doesn't error if directory exists
                 *path_segment = '\\'; // Restore the slash
                 path_segment++; // Move past this segment
             }
             _mkdir(target_path); // Create the final directory
             *last_slash = '\\'; // Restore the full path
         }
         
         switch (changes[i].operation) {
             case SYNC_CREATE:
             case SYNC_MODIFY:
                 if (changes[i].file.is_directory) {
                     // Create directory if it doesn't exist
                     _mkdir(target_path);
                 } else {
                     // Create or modify file
                     HANDLE file_handle = CreateFile(
                         target_path,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                     );
                     
                     if (file_handle != INVALID_HANDLE_VALUE) {
                         DWORD bytes_written;
                         WriteFile(
                             file_handle,
                             changes[i].data,
                             (DWORD)changes[i].data_size,
                             &bytes_written,
                             NULL
                         );
                         CloseHandle(file_handle);
                     } else {
                         printf("Error creating/modifying file: %lu\n", GetLastError());
                     }
                 }
                 break;
                 
             case SYNC_DELETE:
                 if (changes[i].file.is_directory) {
                     // Remove directory
                     RemoveDirectory(target_path);
                 } else {
                     // Delete file
                     DeleteFile(target_path);
                 }
                 break;
         }
         
         free(normalized_source);
     }
     
     free(normalized_target);
 }
 
 // Send changes to the server
 int send_changes_to_server(sync_record *changes, int change_count, const char *server_ip) {
     SOCKET sock;
     struct sockaddr_in server_addr;
     int send_result;
     
     // Create socket
     sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (sock == INVALID_SOCKET) {
         printf("Error creating socket: %d\n", WSAGetLastError());
         return 0;
     }
     
     // Configure server address
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = inet_addr(server_ip);
     server_addr.sin_port = htons(SERVER_PORT);
     
     // Connect to server
     if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
         printf("Error connecting to server: %d\n", WSAGetLastError());
         closesocket(sock);
         return 0;
     }
     
     // Send number of changes
     send_result = send(sock, (char*)&change_count, sizeof(change_count), 0);
     if (send_result == SOCKET_ERROR) {
         printf("Error sending change count: %d\n", WSAGetLastError());
         closesocket(sock);
         return 0;
     }
     
     // Send each change
     for (int i = 0; i < change_count; i++) {
         // Load file data if needed
         if ((changes[i].operation == SYNC_CREATE || changes[i].operation == SYNC_MODIFY) &&
             !changes[i].file.is_directory) {
             HANDLE file_handle = CreateFile(
                 changes[i].file.path,
                 GENERIC_READ,
                 FILE_SHARE_READ,
                 NULL,
                 OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL,
                 NULL
             );
             
             if (file_handle != INVALID_HANDLE_VALUE) {
                 DWORD bytes_read;
                 ReadFile(
                     file_handle,
                     changes[i].data,
                     BUFFER_SIZE,
                     &bytes_read,
                     NULL
                 );
                 changes[i].data_size = bytes_read;
                 CloseHandle(file_handle);
             } else {
                 printf("Error opening file for reading: %lu\n", GetLastError());
                 changes[i].data_size = 0;
             }
         } else {
             changes[i].data_size = 0;
         }
         
         // Send change record
         send_result = send(sock, (char*)&changes[i], sizeof(sync_record), 0);
         if (send_result == SOCKET_ERROR) {
             printf("Error sending change record: %d\n", WSAGetLastError());
             closesocket(sock);
             return 0;
         }
         
         // Send file data if needed
         if (changes[i].data_size > 0) {
             send_result = send(sock, changes[i].data, (int)changes[i].data_size, 0);
             if (send_result == SOCKET_ERROR) {
                 printf("Error sending file data: %d\n", WSAGetLastError());
                 closesocket(sock);
                 return 0;
             }
         }
     }
     
     closesocket(sock);
     return 1;
 }
 
 // Watch directory for changes
 void watch_directory(const char *dir_path, int interval, const char *server_ip) {
     file_info *old_files = NULL, *new_files = NULL;
     int old_count = 0, new_count = 0;
     sync_record *changes = NULL;
     int change_count = 0;
     
     // Initial scan
     scan_directory(dir_path, &old_files, &old_count);
     
     while (1) {
         // Wait for the specified interval
         Sleep(interval * 1000);
         
         // Scan directory again
         scan_directory(dir_path, &new_files, &new_count);
         
         // Detect changes
         compare_directories(old_files, old_count, new_files, new_count, &changes, &change_count);
         
         // Send changes to server if any
         if (change_count > 0) {
             printf("Detected %d changes\n", change_count);
             if (send_changes_to_server(changes, change_count, server_ip)) {
                 printf("Changes sent to server\n");
             } else {
                 printf("Failed to send changes to server\n");
             }
         }
         
         // Free old files and update
         free(old_files);
         old_files = new_files;
         old_count = new_count;
         new_files = NULL;
         
         // Free changes
         free(changes);
         changes = NULL;
     }
 }
 
 int main(int argc, char *argv[]) {
     if (argc < 2) {
         printf("Usage: %s [client|server] [options]\n", argv[0]);
         return 1;
     }
     
     if (strcmp(argv[1], "client") == 0) {
         return client_main(argc, argv);
     } else if (strcmp(argv[1], "server") == 0) {
         return server_main(argc, argv);
     } else {
         printf("Unknown mode: %s\n", argv[1]);
         printf("Usage: %s [client|server] [options]\n", argv[0]);
         return 1;
     }
 }