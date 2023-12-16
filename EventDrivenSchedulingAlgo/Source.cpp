#include <iostream>
#include <queue>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

const int MAX_INT = 2147483647;
const int SWITCH_TIME = 2;

struct Process {
    int process_number;
    int arrival_time;
    double cpu_time;
    double remaining_burst;
    int execution_time;
    int waiting_time;
    int finish_time;
    int turnaround_time;
    bool finished = false;

    Process(int num, int arrival, double cpu)
        : process_number(num), arrival_time(arrival), cpu_time(cpu),
        remaining_burst(cpu), execution_time(0),
        waiting_time(0), finish_time(0), turnaround_time(0) {}
};

class Simulator {
private:
    vector<Process> processes;
    queue<int> ready_queue_fcfs;
    queue<int> finish_queue_fcfs;
    queue<int> ready_queue_rr;
    queue<int> finish_queue_rr;
    int total_switch_time;
    bool isRunningFCFS;

    void readInput(const string& filename);

public:
    Simulator(const string& filename);
    void runFCFS();
    void runRR(int time_quantum);
    void displayProcesses();
    void displayResults(const string& schedulingAlgo);
    void displayReadyQueue(int systemtime);
    void pauseAndInspect(int systemtime);
    void updateQueues(int systemtime);
};

Simulator::Simulator(const string& filename) : total_switch_time(0) {
    readInput(filename);
}

void Simulator::readInput(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    int num_processes;
    file >> num_processes;

    int process_number, arrival_time, cpu_time;
    for (int i = 0; i < num_processes; ++i) {
        file >> process_number >> arrival_time >> cpu_time;
        processes.push_back(Process(process_number, arrival_time, cpu_time));
    }
}



void Simulator::displayProcesses() {
    cout << "Total number of processes: " << processes.size() << "\n";
    cout << "Process Number\tArrival Time\tCPU Time\n";
    for (const auto& process : processes) {
        cout << process.process_number << "\t\t"
            << process.arrival_time << "\t\t"
            << process.cpu_time << "\n";
    }
}

void Simulator::displayResults(const string& schedulingAlgorithm) {
    int total_time = 0;
    int total_waiting_time = 0;
    double total_execution_time = 0;

    for (const auto& process : processes) {
        total_time = max(total_time, process.finish_time);
        total_waiting_time += process.waiting_time;
        total_execution_time += process.cpu_time;
    }

    double average_waiting_time = static_cast<double>(total_waiting_time) / processes.size();
    double cpu_efficiency = total_execution_time / (total_execution_time + total_switch_time) * 100;

    cout << schedulingAlgorithm << ":\n";
    cout << "Total Time: " << total_time << " time units\n";
    cout << "Average Waiting Time: " << average_waiting_time << " time units\n";
    cout << "CPU Efficiency: " << cpu_efficiency << "%\n\n";

    for (const auto& process : processes) {
        cout << "Process " << process.process_number << ":\n";
        cout << "Service time = " << process.cpu_time << "\n";
        cout << "Waiting time = " << process.waiting_time << "\n";
        cout << "Finish time = " << process.finish_time << "\n";
        cout << "Turnaround time = " << process.turnaround_time << "\n\n";
    }
}


void Simulator::displayReadyQueue(int systemtime) {
    cout << "Ready Queue at time " << systemtime << ": ";
    queue<int> temp = isRunningFCFS ? ready_queue_fcfs : ready_queue_rr;
    while (!temp.empty()) {
        int processIndex = temp.front();
        temp.pop();
        cout << "P" << processes[processIndex].process_number << " ";
    }
    cout << endl;
}

void Simulator::pauseAndInspect(int systemtime) {
    cout << "\n[EVENT] Current System Time: " << systemtime << "\n";
    displayReadyQueue(systemtime);
    cout << "Finish Queue: ";
    queue<int> temp_finish = isRunningFCFS ? finish_queue_fcfs : finish_queue_rr;
    while (!temp_finish.empty()) {
        int processIndex = temp_finish.front();
        temp_finish.pop();
        cout << "P" << processes[processIndex].process_number << " ";
    }
    cout << "\n\n";
}


void Simulator::updateQueues(int systemtime) {
    queue<int>().swap(ready_queue_fcfs);
    queue<int>().swap(finish_queue_fcfs);

    for (int i = 0; i < processes.size(); ++i) {
        if (!processes[i].finished && processes[i].arrival_time <= systemtime) {
            ready_queue_fcfs.push(i);
        }
        if (processes[i].finished && processes[i].finish_time <= systemtime) {
            finish_queue_fcfs.push(i);
        }
    }
}

void Simulator::runFCFS() {
    isRunningFCFS = true;
    total_switch_time = 0;

    for (auto& process : processes) {
        process.finished = false;
        process.execution_time = 0;
        process.waiting_time = 0;
        process.finish_time = 0;
        process.turnaround_time = 0;
    }

    queue<int>().swap(ready_queue_fcfs);
    queue<int>().swap(finish_queue_fcfs);

    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrival_time < b.arrival_time;
    });

    int currentTime = 0;
    for (auto& process : processes) {
        if (currentTime < process.arrival_time) {
            currentTime = process.arrival_time;
        }

        updateQueues(currentTime);
        pauseAndInspect(currentTime);

        process.execution_time = currentTime;
        process.waiting_time = process.execution_time - process.arrival_time;
        currentTime += process.cpu_time;
        process.finish_time = currentTime;
        process.turnaround_time = process.finish_time - process.arrival_time;
        process.finished = true;

        updateQueues(currentTime);
        pauseAndInspect(currentTime);

        if (&process != &processes.back()) {
            currentTime += SWITCH_TIME;
            total_switch_time += SWITCH_TIME;
        }
    }
}

void Simulator::runRR(int time_quantum) {
    isRunningFCFS = false;
    total_switch_time = 0;
    queue<int>().swap(ready_queue_rr);
    queue<int>().swap(finish_queue_rr);

    for (auto& process : processes) {
        process.finished = false;
        process.execution_time = 0;
        process.waiting_time = 0;
        process.finish_time = 0;
        process.turnaround_time = 0;
        process.remaining_burst = process.cpu_time;
    }

    int currentTime = 0;
    int nextProcessToCheck = 0;

    while (nextProcessToCheck < processes.size() && processes[nextProcessToCheck].arrival_time <= currentTime) {
        ready_queue_rr.push(nextProcessToCheck);
        nextProcessToCheck++;
    }

    while (finish_queue_rr.size() < processes.size()) {
        if (!ready_queue_rr.empty()) {
            int processIndex = ready_queue_rr.front();
            ready_queue_rr.pop();
            Process& job = processes[processIndex];

            pauseAndInspect(currentTime);

            int timeSpent = min(time_quantum, static_cast<int>(job.remaining_burst));
            job.remaining_burst -= timeSpent;
            currentTime += timeSpent;

            if (job.remaining_burst > 0) {
                ready_queue_rr.push(processIndex);
            }
            else {
                job.finish_time = currentTime;
                job.turnaround_time = job.finish_time - job.arrival_time;
                job.waiting_time = job.turnaround_time - job.cpu_time;
                finish_queue_rr.push(processIndex);
                job.finished = true;

                pauseAndInspect(currentTime);
            }

            if (!ready_queue_rr.empty() || job.remaining_burst > 0) {
                currentTime += SWITCH_TIME;
                total_switch_time += SWITCH_TIME;
            }
        }
        else {
            currentTime++;
        }

        while (nextProcessToCheck < processes.size() && processes[nextProcessToCheck].arrival_time <= currentTime) {
            ready_queue_rr.push(nextProcessToCheck);
            nextProcessToCheck++;
        }
    }
}


int main() {
    Simulator simulator("processes.txt");
    //simulator.displayProcesses(); // just a check to see if input is read correctly
    simulator.runFCFS();
    simulator.displayResults("First Come First Serve (non-preemptive)");
    simulator.runRR(50);
    simulator.displayResults("Round Robin (preemptive)");
    return 0;
}
