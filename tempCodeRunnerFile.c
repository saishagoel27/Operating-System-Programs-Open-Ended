#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <psapi.h>

// Structure to hold system resource data
typedef struct {
    time_t timestamp;
    double cpu_usage;
    double memory_usage_percent;
    DWORDLONG memory_total;
    DWORDLONG memory_available;
    DWORDLONG disk_read_bytes;
    DWORDLONG disk_write_bytes;
    // Network stats would require additional libraries
} SystemResources;

// Function to get CPU usage (uses PDH in a real implementation)
double get_cpu_usage() {
    // This is a simplified version - a real implementation would use PDH
    // For demonstration, we return a number between 0-100
    static int last_value = 50;
    // Generate a value that somewhat resembles real CPU usage patterns
    int change = rand() % 11 - 5; // -5 to +5
    last_value += change;
    
    if (last_value > 100) last_value = 100;
    if (last_value < 0) last_value = 0;
    
    return (double)last_value;
}

// Function to get memory information
void get_memory_info(DWORDLONG *total, DWORDLONG *available, double *percent) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    *total = memInfo.ullTotalPhys;
    *available = memInfo.ullAvailPhys;
    *percent = memInfo.dwMemoryLoad; // Already as a percentage
}

// Function to get disk I/O information
void get_disk_io(DWORDLONG *read_bytes, DWORDLONG *write_bytes) {
    // In a real implementation, you would use GetDiskIoInformation or similar
    // For demonstration, we return simulated values
    static DWORDLONG last_read = 1000000;
    static DWORDLONG last_write = 500000;
    
    *read_bytes = last_read + (rand() % 100000);
    *write_bytes = last_write + (rand() % 50000);
    
    last_read = *read_bytes;
    last_write = *write_bytes;
}

// Function to collect all system resource information
SystemResources collect_system_resources() {
    SystemResources res;
    
    // Set timestamp
    res.timestamp = time(NULL);
    
    // Collect CPU usage
    res.cpu_usage = get_cpu_usage();
    
    // Collect memory information
    get_memory_info(&res.memory_total, &res.memory_available, &res.memory_usage_percent);
    
    // Collect disk I/O information
    get_disk_io(&res.disk_read_bytes, &res.disk_write_bytes);
    
    return res;
}

// Function to log resource data to a CSV file
void log_resources_to_file(SystemResources res, FILE *file) {
    char timestamp_str[30];
    struct tm *tm_info = localtime(&res.timestamp);
    strftime(timestamp_str, 30, "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(file, "%s,%.2f,%.2f,%llu,%llu,%llu,%llu\n",
            timestamp_str,
            res.cpu_usage,
            res.memory_usage_percent,
            res.memory_total,
            res.memory_available,
            res.disk_read_bytes,
            res.disk_write_bytes);
}

// Function to display current system resources
void display_current_resources(SystemResources res) {
    system("cls"); // Clear console
    
    char timestamp_str[30];
    struct tm *tm_info = localtime(&res.timestamp);
    strftime(timestamp_str, 30, "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("===== SYSTEM RESOURCE MONITOR =====\n");
    printf("Time: %s\n\n", timestamp_str);
    
    // Display CPU usage with a simple bar graph
    printf("CPU Usage: %.2f%%\t[", res.cpu_usage);
    int bars = (int)(res.cpu_usage / 5);
    for (int i = 0; i < 20; i++) {
        if (i < bars) {
            printf("|");
        } else {
            printf(" ");
        }
    }
    printf("]\n\n");
    
    // Display memory usage
    double used_memory_gb = (res.memory_total - res.memory_available) / (1024.0 * 1024 * 1024);
    double total_memory_gb = res.memory_total / (1024.0 * 1024 * 1024);
    
    printf("Memory Usage: %.2f%%\t[", res.memory_usage_percent);
    bars = (int)(res.memory_usage_percent / 5);
    for (int i = 0; i < 20; i++) {
        if (i < bars) {
            printf("|");
        } else {
            printf(" ");
        }
    }
    printf("]\n");
    printf("Memory Used: %.2f GB / %.2f GB\n\n", used_memory_gb, total_memory_gb);
    
    // Display disk I/O
    printf("Disk Read: %.2f MB\n", res.disk_read_bytes / (1024.0 * 1024));
    printf("Disk Write: %.2f MB\n\n", res.disk_write_bytes / (1024.0 * 1024));
    
    printf("Press Ctrl+C to exit...\n");
}

int main() {
    // Set console title
    SetConsoleTitle("System Resource Monitor");
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Create log file
    FILE *log_file = fopen("system_resources.csv", "w");
    if (log_file == NULL) {
        printf("Error creating log file\n");
        return 1;
    }
    
    // Write CSV header
    fprintf(log_file, "Timestamp,CPU Usage %%,Memory Usage %%,Memory Total,Memory Available,Disk Read Bytes,Disk Write Bytes\n");
    
    printf("System Resource Monitor Started\n");
    printf("Logging to system_resources.csv\n");
    
    while (1) {
        // Collect system resources
        SystemResources resources = collect_system_resources();
        
        // Display in console
        display_current_resources(resources);
        
        // Log to file
        log_resources_to_file(resources, log_file);
        
        // Flush file buffer to ensure data is written
        fflush(log_file);
        
        // Wait for 1 second
        Sleep(1000);
    }
    
    // Close log file (this won't actually execute due to infinite loop)
    fclose(log_file);
    
    return 0;
}