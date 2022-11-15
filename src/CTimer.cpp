
#include "CTimer.h"

CTimer::CTimer(TimerHandler timer_handler)
    : timer_handler_(timer_handler)
    , mode_(CTimer::Mode::kOneshot)
    , exit_requested_(false)
    , running_(false)
{
    thread_ = std::thread(&CTimer::Run, this);
}

CTimer::~CTimer()
{
    exit_requested_ = true;
    Stop();
    // Lift the thread out of its waiting state
    condition_variable_.notify_all();
    thread_.join();
}

void CTimer::StartOnce(CTimer::Clock::duration timeout, const bool flag)
{
    
    std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kOneshot;
    next_expiry_point_ = Clock::now() + timeout;
    flag_ = flag;
    running_ = true;
    condition_variable_.notify_all();
}

void CTimer::StartPeriodicImmediate(CTimer::Clock::duration period, const bool flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now();
    period_ = period;
    flag_ = flag;
    running_ = true;
    condition_variable_.notify_all();
}

void CTimer::StartPeriodicDelayed(CTimer::Clock::duration delayperiod, CTimer::Clock::duration period, const bool flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now() + delayperiod;
    period_ = period;
    flag_ = flag;
    running_ = true;
    condition_variable_.notify_all();
}

void CTimer::Run()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!exit_requested_) {
        // Block until timer expires or timer is started again
        if (running_) {
            condition_variable_.wait_until(lock, next_expiry_point_);
        } else {
            condition_variable_.wait(lock);
        }
//        std::cout << "Timer mode: " << mode_ << std::endl;
        // Timer was woken. Determine why.
        if (running_) {
            if (next_expiry_point_ <= Clock::now()) {
                // Unlock the mutex during a potentially long-running operation
                lock.unlock();
                timer_handler_(flag_);
                lock.lock();
                // Determine if we have to set the timer again
                if (kOneshot == mode_) {
                    running_ = false;
                } else {
                    next_expiry_point_ += period_;
                }
            }
        }
    }
}