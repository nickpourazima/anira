#include <anira/system/RealtimeThread.h>

namespace anira {

RealtimeThread::RealtimeThread() : m_should_exit(false) {
}

RealtimeThread::~RealtimeThread() {
    stop();
}

void RealtimeThread::start() {
    m_should_exit = false;
    #if __linux__ && !defined(__ANDROID__)
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
        pthread_setattr_default_np(&thread_attr);
    #endif

    thread = std::thread(&RealtimeThread::run, this);

    #if __linux__ && !defined(__ANDROID__)
        pthread_attr_destroy(&thread_attr);
    #endif

    elevateToRealTimePriority(thread.native_handle());
    }

void RealtimeThread::stop() {
    m_should_exit = true;
    if (thread.joinable()) thread.join();
}   

void RealtimeThread::elevateToRealTimePriority(std::thread::native_handle_type thread_native_handle, bool is_main_process) {
#if WIN32
    if (is_main_process) {
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
           std::cerr << "[ERROR] Failed to set real-time priority for process. Error: " << GetLastError() << std::endl;
        }
    }

    int priorities[] = {THREAD_PRIORITY_TIME_CRITICAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_ABOVE_NORMAL};
    for (int priority : priorities) {
        if (SetThreadPriority(thread_native_handle, priority)) {
            return;
        } else {
            std::cerr << "[ERROR] Failed to set thread priority for Thread. Current priority: " << priority << std::endl;
        }
    }
#elif __linux__
    int ret;

    if (!is_main_process) {
        int attr_inheritsched;
        pthread_attr_t thread_attr;
        ret = pthread_getattr_np(thread_native_handle, &thread_attr);
        ret = pthread_attr_getinheritsched(&thread_attr, &attr_inheritsched);
        if(ret != 0) {
            std::cerr << "[ERROR] Failed to get Thread scheduling policy and params : " << errno << std::endl;\
        }
        if (attr_inheritsched != PTHREAD_EXPLICIT_SCHED) {
            std::cerr << "[ERROR] Thread scheduling policy is not PTHREAD_EXPLICIT_SCHED. Possibly thread attributes get inherited from the main process." << std::endl;
        }
        pthread_attr_destroy(&thread_attr);
    }

    int sch_policy;
    struct sched_param sch_params;

    ret = pthread_getschedparam(thread_native_handle, &sch_policy, &sch_params);
    if(ret != 0) {
        std::cerr << "[ERROR] Failed to get Thread scheduling policy and params : " << errno << std::endl;
    }

    // Pipewire uses SCHED_FIFO 60 and juce plugin host uses SCHED_FIFO 55 better stay below
    sch_params.sched_priority = 50;

    ret = pthread_setschedparam(thread_native_handle, SCHED_FIFO, &sch_params); 
    if(ret != 0) {
        std::cerr << "[ERROR] Failed to set Thread scheduling policy to SCHED_FIFO and increase the sched_priority to " << sch_params.sched_priority << ". Error : " << errno << std::endl;
        std::cout << "[WARNING] Give rtprio privileges to the user by adding the user to the realtime/audio group. Or run the application as root." << std::endl;
        std::cout << "[WARNING] Instead, trying to set increased nice value for SCHED_OTHER..." << std::endl;
    }

    // TODO Check if PRIO_PROCESS is the right who to set the nice value, since it should only affect the current process id, still it works
    ret = setpriority(PRIO_PROCESS, 0, -10);

    if(ret != 0) {
        std::cerr << "[ERROR] Failed to set increased nice value. Error : " << errno << std::endl;
        std::cout << "[WARNING] Using default nice value: " << getpriority(PRIO_PROCESS, 0) << std::endl;
    }
    return;
#elif __APPLE__
    int ret;

    ret = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (ret != 0) {
        std::cerr << "[ERROR] Failed to set Thread QOS class to QOS_CLASS_USER_INTERACTIVE. Error : " << ret << std::endl;
    } else {
        return;
    }

    std::cerr << "[ERROR] Failed to set Thread QOS class and relative priority. Error: " << ret << std::endl;

    qos_class_t qos_class;
    int relative_priority;
    pthread_get_qos_class_np(pthread_self(), &qos_class, &relative_priority);

    std::cout << "[WARNING] Fallback to default QOS class and relative priority: " << qos_class << " " << relative_priority << std::endl; 
    return;
#endif
}

#if defined(HAVE_ANDROID_PTHREAD_SETNAME_NP)
void setThreadName(const char* name) {
    char buf[16]; // MAX_TASK_COMM_LEN=16 is hard-coded into bionic
    strncpy(buf, name, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    int err = pthread_setname_np(pthread_self(), buf);
    if (err != 0) {
        std::cerr << "Unable to set the name of current thread to '" << buf << "': " << strerror(err) << std::endl;
    }
}
#elif defined(HAVE_PRCTL)
void setThreadName(const char* name) {
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
}
#else
void setThreadName(const char* name) {
    std::cout << "No way to set current thread's name (" << name << ")" << std::endl;
}
#endif

bool RealtimeThread::shouldExit() {
    return m_should_exit;
}

} // namespace anira