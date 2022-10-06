#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include <sys/resource.h>
#include <unistd.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <memory>
#include <stdexcept>
#include <array>
#include <sys/klog.h>
#include <cstring>

#define DO_CONTROL

struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags){
    return syscall(SYS_sched_setattr, pid, attr, flags);
}
int sched_getattr(pid_t pid, const struct sched_attr *attr, unsigned int size, unsigned int flags){
    return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

/* constants for PID */
const float Kp_U = 0.185;
const float Kp_M = 0.148;
//const float Ki = 0.01;
//const float Kd = 0.001;
//const int Set_Point = 353;

/* constants for reference */
const float M_s = 10; // Miss ratio reference
const float U_s = 0.8; // Utilization reference

const int QoS_levels = 2;
const int which = PRIO_PROCESS;
int cycles;
std::vector<int> tasks;
float CPU_now = 0.0;
float CPU_now_idle = 0.0;

void AssignQoS(float U, float M, float B_ratio);

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;

    auto pipe = popen(cmd, "r"); // get rid of shared_ptr

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
            result += buffer.data();
    }

    auto rc = pclose(pipe);

    if (rc == EXIT_SUCCESS) { // == 0

    } else if (rc == EXIT_FAILURE) {  // EXIT_FAILURE is not used by all programs, maybe needs some adaptation.

    }
    return result;
}

float read_CPU_Usage();
float read_Miss_Ratio();

// The feedback control RT controller takes CPU utilization and miss ratio as input
// to calculate the adjust QoS (B) for current cycle. The controller will adjust each
// tasks' attribute if QoS decrease. If the CPU utilization is under threshold,
// we increase each tasks' runtime by New_QoS/Old_QoS. If the miss ratio > 0,
// we decrease each tasks' deadline by New_QoS/Old_QoS.
void Controller()
{
    float U = 0.0, M = 0.0, B = 0.0, old_B = B; // CPU Utilization, Miss ratio, QoS on system level
    float E_M, D_BM, E_U, D_BU, D_B;
    std::vector<int> tasks;
    std::ofstream report_file;
    int cycle_count=0;
    report_file.open("log/report.csv");
    report_file << "second, CPU_Usage, Miss_Count, QoS\n";

    int ret;
    struct sched_attr attr;

    ret = sched_getattr(0, &attr, sizeof(attr), 0);
    if (ret < 0){
            perror("controller getattr failed.");
            return;
    }

    attr.sched_policy = SCHED_DEADLINE;
    //attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_runtime = 1000*1000;
    attr.sched_period = 1000*1000*1000;
    attr.sched_deadline = attr.sched_period;

    ret = sched_setattr(0, &attr, 0);
    if (ret < 0){
            perror("controller setattr failed.");
            return;
    }
    tasks.push_back(0);

    while (cycles==0 || cycle_count<cycles) {
	old_B = B;

        U = read_CPU_Usage();
	M = read_Miss_Ratio();

	std::cout << "U=" << U << ", M=" << M << std::endl;
        #ifdef DO_CONTROL
	//std::cout << "do controll" << std::endl;
	E_M = M_s - M;
        D_BM = Kp_M * E_M;
        E_U = U_s - U;
        D_BU = Kp_U * E_U;
        D_B = std::min(D_BM, D_BU);

	B = B + D_B;
	std::cout << "B=" << B << ", old_B=" << old_B << std::endl;
	if (B<old_B) AssignQoS(U, M, B/old_B);
        #endif

        report_file << cycle_count << ", " << U*100 << ", " << M << ", " << B << std::endl;
	cycle_count++;

        sched_yield();
    }
    report_file.close();
}

float read_CPU_Usage()
{
    float U = 0.0, CPU = 0.0, idle = 0.0, used = 0.0;
    unsigned u, n, s, id, io, irq, sirq;
    
    if (std::ifstream("/proc/stat").ignore(3) >> u >> n >> s >> id >> io >> irq >> sirq) {
	//std::cout << "u=" << u << ", n=" << n << ", s=" << s << ", id=" << id << ", io=" << io << ", irq=" << irq << ", sirq=" << sirq << std::endl;
        //CPU = (float)(u+n+s+id+io+irq+sirq);
	CPU = (float)(u+n+s+id);
	idle = (float) id;
	used = (CPU - CPU_now) - (idle - CPU_now_idle);
	U = used/(CPU-CPU_now);
	CPU_now = CPU;
	CPU_now_idle = idle;
    }

    return U;
}

float read_Miss_Ratio()
{
    float count=0;
    //std::string res = exec("dmesg | grep -i miss 1>&2");
    char t[1];
    int len = klogctl(10, t, 0);
    char res[len];
    char *token, *check_miss;
    int ret;
    ret = klogctl(4, res, len);
    res[ret] = '\0';
    //std::cout << res << std::endl;
    token = strtok(res, "\n");
    while (token!=NULL){
	/*
        for (int i=0;i<ret;i++)
	    if (res[i]=='\n') {
                //std::cout << i << std::endl;
	        count++;
	    }
	*/
	check_miss = strstr(token, "miss");
	//std::cout << "check_miss: " << check_miss << std::endl;
	if (check_miss) count++;
	token = strtok(NULL, "\n");
    }
    //res = exec("dmesg -c");
    //std::cout << "miss count = " << count << std::endl;
    return count;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: ./PIController cycles_in_second tasks_list_file" << std::endl;
        return -1;
    }

    std::string line;
    std::ifstream tasks_file;
    std::stringstream ss;
    int pid;
    //std::cout << argv[2] << std::endl;
    tasks_file.open(argv[2]);
    while (std::getline(tasks_file, line)) {
	ss << line;
        ss >> pid;
        tasks.push_back(pid);
    }
    tasks_file.close();
    //std::cout << tasks.size() << std::endl;
    cycles = atoi(argv[1]);
    //std::cout << cycles << std::endl;

    Controller();

    return 0;
}

void AssignQoS(float U, float M, float B_ratio)
{
    int ret;
    struct sched_attr attr;
    int new_deadline;
    int new_runtime;
    //std::String out[tasks.size()];
    // Migrated Container first
    if (M>0) {
	std::cout << "reduce deadline by " << B_ratio << std::endl;
        for (int i=0;i<tasks.size();i++) {
            //if(tasks[i] == ) ret = setpriority(which, pid, -20);
            //else if(tasks[i] == "1") ret = setpriority(which, pid, 0);
            //else ret = setpriority(which, pid, 20);
            ret = sched_getattr(tasks[i], &attr, sizeof(attr), 0);
	    new_deadline = attr.sched_deadline * (1-B_ratio);
	    if (new_deadline > attr.sched_runtime) {
	        attr.sched_deadline = new_deadline;
	        ret = sched_setattr(tasks[i], &attr, 0);
                if (ret < 0){
		    std::cerr << tasks[i] << ": setattr failed." << std::endl;
                    //return;
                }
	    }
        }
    }
    else {
        std::cout << "increase runtime by " << B_ratio << std::endl;
        for (int i=0;i<tasks.size();i++) {
            //if(tasks[i] == ) ret = setpriority(which, pid, -20);
            //else if(tasks[i] == "1") ret = setpriority(which, pid, 0);
            //else ret = setpriority(which, pid, 20);
            ret = sched_getattr(tasks[i], &attr, sizeof(attr), 0);
            new_runtime = attr.sched_runtime * (1+B_ratio);
	    if (new_runtime < attr.sched_deadline) {
		attr.sched_runtime = new_runtime;
	        ret = sched_setattr(tasks[i], &attr, 0);
                if (ret < 0){
                    std::cerr << tasks[i] << ": setattr failed." << std::endl;
                    //return;
                }
	    }
        }
    }
}
