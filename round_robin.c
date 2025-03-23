#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESS_NAME 20
#define MAX_PROCESSES 100

// Process structure
typedef struct {
    char name[MAX_PROCESS_NAME];
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    bool started;
} Process;

// Queue structure for ready queue
typedef struct {
    Process* processes[MAX_PROCESSES];
    int front;
    int rear;
    int size;
} Queue;

// Function prototypes
void initialize_queue(Queue* q);
bool is_queue_empty(Queue* q);
bool is_queue_full(Queue* q);
void enqueue(Queue* q, Process* process);
Process* dequeue(Queue* q);
void round_robin_scheduler(Process processes[], int n, int time_quantum);
void print_gantt_chart(Process* execution_order[], int execution_times[], int n);
void print_process_details(Process processes[], int n);
void calculate_average_times(Process processes[], int n);
void generate_random_processes(Process processes[], int n);

// Generate a random integer between min and max (inclusive)
int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

int main() {
    srand(time(NULL)); // Seed for random number generation
    
    int n, time_quantum;
    char choice;
    
    printf("Round Robin CPU Scheduling Simulator\n");
    printf("===================================\n\n");
    
    printf("Do you want to generate random processes? (y/n): ");
    scanf(" %c", &choice);
    
    printf("Enter the number of processes: ");
    scanf("%d", &n);
    
    if (n <= 0 || n > MAX_PROCESSES) {
        printf("Invalid number of processes. Must be between 1 and %d.\n", MAX_PROCESSES);
        return 1;
    }
    
    Process processes[n];
    
    if (choice == 'y' || choice == 'Y') {
        generate_random_processes(processes, n);
    } else {
        // Manual input
        for (int i = 0; i < n; i++) {
            printf("\nProcess %d:\n", i + 1);
            
            sprintf(processes[i].name, "P%d", i + 1);
            processes[i].pid = i + 1;
            
            printf("Enter arrival time for %s: ", processes[i].name);
            scanf("%d", &processes[i].arrival_time);
            
            printf("Enter burst time for %s: ", processes[i].name);
            scanf("%d", &processes[i].burst_time);
            
            processes[i].remaining_time = processes[i].burst_time;
            processes[i].completion_time = 0;
            processes[i].started = false;
        }
    }
    
    printf("\nEnter time quantum: ");
    scanf("%d", &time_quantum);
    
    if (time_quantum <= 0) {
        printf("Time quantum must be greater than 0.\n");
        return 1;
    }
    
    // Execute the round robin scheduler
    round_robin_scheduler(processes, n, time_quantum);
    
    // Calculate and print results
    print_process_details(processes, n);
    calculate_average_times(processes, n);
    
    return 0;
}

void initialize_queue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

bool is_queue_empty(Queue* q) {
    return q->size == 0;
}

bool is_queue_full(Queue* q) {
    return q->size == MAX_PROCESSES;
}

void enqueue(Queue* q, Process* process) {
    if (is_queue_full(q)) {
        printf("Queue is full. Cannot enqueue process %s.\n", process->name);
        return;
    }
    
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->processes[q->rear] = process;
    q->size++;
}

Process* dequeue(Queue* q) {
    if (is_queue_empty(q)) {
        return NULL;
    }
    
    Process* process = q->processes[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->size--;
    
    return process;
}

void round_robin_scheduler(Process processes[], int n, int time_quantum) {
    Queue ready_queue;
    initialize_queue(&ready_queue);
    
    int current_time = 0;
    int completed = 0;
    bool in_queue[n];
    memset(in_queue, false, sizeof(in_queue));
    
    // Arrays to store execution order for Gantt chart
    Process* execution_order[1000]; // Assuming no more than 1000 context switches
    int execution_times[1000];
    int execution_count = 0;
    
    while (completed < n) {
        // Check for newly arrived processes
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time <= current_time && processes[i].remaining_time > 0 && !in_queue[i]) {
                enqueue(&ready_queue, &processes[i]);
                in_queue[i] = true;
            }
        }
        
        if (is_queue_empty(&ready_queue)) {
            current_time++; // If no process is ready, advance time
            continue;
        }
        
        // Get the next process from ready queue
        Process* current_process = dequeue(&ready_queue);
        int process_index = current_process->pid - 1;
        in_queue[process_index] = false;
        
        // Record for Gantt chart
        execution_order[execution_count] = current_process;
        execution_times[execution_count] = current_time;
        execution_count++;
        
        // Execute process for time quantum or until completion
        int execution_time = (current_process->remaining_time < time_quantum) ? 
                              current_process->remaining_time : time_quantum;
        
        // If process is executing for the first time, record response time
        if (!current_process->started) {
            current_process->response_time = current_time - current_process->arrival_time;
            current_process->started = true;
        }
        
        // Update current time and remaining time
        current_time += execution_time;
        current_process->remaining_time -= execution_time;
        
        // Check for newly arrived processes during execution
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time > execution_times[execution_count-1] && 
                processes[i].arrival_time <= current_time && 
                processes[i].remaining_time > 0 && !in_queue[i]) {
                enqueue(&ready_queue, &processes[i]);
                in_queue[i] = true;
            }
        }
        
        // If process is completed
        if (current_process->remaining_time == 0) {
            completed++;
            current_process->completion_time = current_time;
            current_process->turnaround_time = current_process->completion_time - current_process->arrival_time;
            current_process->waiting_time = current_process->turnaround_time - current_process->burst_time;
        } else {
            // If process is not completed, put it back in the ready queue
            enqueue(&ready_queue, current_process);
            in_queue[process_index] = true;
        }
    }
    
    // Add final time for Gantt chart
    execution_times[execution_count] = current_time;
    
    // Print Gantt chart
    print_gantt_chart(execution_order, execution_times, execution_count);
}

void print_gantt_chart(Process* execution_order[], int execution_times[], int n) {
    printf("\nGantt Chart:\n");
    
    // Print top border
    printf(" ");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < execution_times[i+1] - execution_times[i]; j++) {
            printf("--");
        }
        printf(" ");
    }
    printf("\n|");
    
    // Print process names
    for (int i = 0; i < n; i++) {
        int duration = execution_times[i+1] - execution_times[i];
        
        // Calculate padding
        int name_length = strlen(execution_order[i]->name);
        int padding = (2 * duration - name_length) / 2;
        
        for (int j = 0; j < padding; j++) {
            printf(" ");
        }
        
        printf("%s", execution_order[i]->name);
        
        for (int j = 0; j < 2 * duration - name_length - padding; j++) {
            printf(" ");
        }
        
        printf("|");
    }
    printf("\n ");
    
    // Print bottom border
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < execution_times[i+1] - execution_times[i]; j++) {
            printf("--");
        }
        printf(" ");
    }
    printf("\n");
    
    // Print timeline
    printf("0");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < execution_times[i+1] - execution_times[i] - 1; j++) {
            printf("  ");
        }
        printf("%2d", execution_times[i+1]);
    }
    printf("\n");
}

// Function to print process details table
void print_process_details(Process processes[], int n) {
    printf("\nProcess Details:\n");
    printf("+------+---------------+------------+-------------+----------------+---------------+-------------+\n");
    printf("| PID  | Process Name  | Arrival Time | Burst Time | Completion Time | Turnaround Time | Waiting Time | Response Time |\n");
    printf("+------+---------------+------------+-------------+----------------+---------------+-------------+\n");
    
    for (int i = 0; i < n; i++) {
        printf("| %4d | %-13s | %10d | %11d | %14d | %15d | %11d | %12d |\n",
               processes[i].pid,
               processes[i].name,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].completion_time,
               processes[i].turnaround_time,
               processes[i].waiting_time,
               processes[i].response_time);
    }
    
    printf("+------+---------------+------------+-------------+----------------+---------------+-------------+\n");
}

// Function to calculate and print average times
void calculate_average_times(Process processes[], int n) {
    float avg_turnaround_time = 0;
    float avg_waiting_time = 0;
    float avg_response_time = 0;
    
    for (int i = 0; i < n; i++) {
        avg_turnaround_time += processes[i].turnaround_time;
        avg_waiting_time += processes[i].waiting_time;
        avg_response_time += processes[i].response_time;
    }
    
    avg_turnaround_time /= n;
    avg_waiting_time /= n;
    avg_response_time /= n;
    
    printf("\nAverage Turnaround Time: %.2f\n", avg_turnaround_time);
    printf("Average Waiting Time: %.2f\n", avg_waiting_time);
    printf("Average Response Time: %.2f\n", avg_response_time);
}

// Function to generate random processes
void generate_random_processes(Process processes[], int n) {
    printf("\nGenerating %d random processes...\n", n);
    
    for (int i = 0; i < n; i++) {
        sprintf(processes[i].name, "P%d", i + 1);
        processes[i].pid = i + 1;
        processes[i].arrival_time = random_int(0, 10);
        processes[i].burst_time = random_int(1, 20);
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].completion_time = 0;
        processes[i].started = false;
        
        printf("Generated %s: Arrival Time = %d, Burst Time = %d\n",
               processes[i].name,
               processes[i].arrival_time,
               processes[i].burst_time);
    }
}