# TaskScheduler
TasksScheduler enables the scheduling and execution of tasks of different types (periodic, trigger, or once). The tasks are executed using the boost::asio library, which provides asynchronous I/O and timers.

Example usage:

```
#include "TasksScheduler.h"
#include <functional>
#include <iostream>

void periodicTaskFn()
{
    std::cout << "Executing periodic task." << std::endl;
}

void triggerTaskFn()
{
    std::cout << "Executing trigger task." << std::endl;
}

void onceTaskFn()
{
    std::cout << "Executing once task." << std::endl;
}

int main()
{
    boost::asio::io_service ioService;

    TasksScheduler<std::string, std::function<void()>> tasksScheduler;

    tasksScheduler.addTask("PeriodicTask", 1000, ME9_TypeTask::PeriodicTaskType, std::bind(&periodicTaskFn));
    tasksScheduler.addTask("TriggerTask", 0, ME9_TypeTask::TriggerTaskType, std::bind(&triggerTaskFn));
    tasksScheduler.addTask("OnceTask", 5000, ME9_TypeTask::OnceTaskType, std::bind(&onceTaskFn));

    while (1) {
        tasksScheduler.pollOne();
    }

    return 0;
}
```

Output:

```
Executing periodic task.
Executing periodic task.
Executing periodic task.
Executing periodic task.
Executing periodic task.
Executing once task.
Executing periodic task.
Executing periodic task.
```
