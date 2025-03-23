#include <stdio.h>
#include <windows.h>

int main() {
    printf("Note: True zombie processes don't exist in Windows the same way they do in Unix/Linux.\n");
    printf("Windows automatically cleans up terminated processes.\n\n");
    
    printf("Creating some child processes instead...\n");
    
    for (int i = 0; i < 5; i++) {
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        
        // Create a process that runs the command prompt
        if (CreateProcess(NULL, "cmd.exe /c timeout 10", NULL, NULL, FALSE, 
                         CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            printf("Created child process %d with PID: %d\n", i+1, pi.dwProcessId);
            
            // Don't wait for it to complete - similar to not calling wait()
            // CloseHandle(pi.hProcess); // This would create the closest thing to a zombie in Windows
            
            // But we keep the handle open to demonstrate the difference
        }
        else {
            printf("Failed to create process %d\n", i+1);
        }
        
        Sleep(1000); // Wait 1 second between processes
    }
    
    printf("\nIn Windows, processes are automatically cleaned up when they terminate.\n");
    printf("Press Enter to exit...\n");
    getchar();
    
    return 0;
}
