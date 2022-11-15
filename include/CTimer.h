
#ifndef _COMMON_TIMER_H_
#define _COMMON_TIMER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

/**
 * \brief Implements a Timer that owns a separate thread.
 *
 * The thread is running as long as the timer is running. However, it
 * consumes CPU time only when the timer
 * fires, is stopped, or its expiration point is updated.
 *
 * Note that it is guaranteed that no locks are held while the stored
 * TimerHandler is called.
 */
class CTimer
{
public:
    /**
     * \brief Typedef for the chrono clock used by this Timer.
     */
    typedef std::chrono::steady_clock Clock;

    /**
     * \brief Typedef for the callback called when the timer fires.
     */
    using TimerHandler = std::function<void(bool)>;

    /**
     * \brief Constructor to build a new Timer. The new timer is stopped.
     *
     * \param timer_handler The callback to call when the timer expires.
     */
    explicit CTimer(TimerHandler timer_handler);

    /**
     * \brief Destructor that implicitly stops the timer and joins the timer
     * handler thread.
     */
    virtual ~CTimer();

    /**
     * \brief Start a timer with the specified delay that stops itself after it
     * fires.
     */
    void StartOnce(Clock::duration timeout, const bool flag = false);

    /**
     * \brief Start a periodic timer that fires immediately.
     */
    void StartPeriodicImmediate(Clock::duration period, const bool flag = false);

    /**
     * \brief Start a periodic timer that first fires after one delayperiod has passed.
     */
    void StartPeriodicDelayed(Clock::duration delayperiod, Clock::duration period, const bool flag = false);

    /**
     * \brief Determine whether the timer is currently running, i.e., will fire at
     * some point in the future.
     */
    inline bool IsRunning() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }

    /**
     * \brief Determine whether the timer is periodic, i.e., will fire more than
     * once at some point in the future.
     */
    inline bool IsPeriodic() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return kPeriodic == mode_;
    }

    /**
     * \brief Stop the timer.
     */
    inline void Stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }

    /**
     * \brief Returns the time until the next expiry of the timer. This value is
     * only meaningful if is_running() == true.
     */
    inline Clock::time_point GetNextExpiryPoint() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return next_expiry_point_;
    }

    /**
     * \brief Returns the next time the timer will expire. This value is only
     * meaningful if is_running() == true &&
     * is_periodic() == true.
     */
    inline Clock::duration GetTimeRemaining() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return next_expiry_point_ - Clock::now();
    }

protected:
    /**
     * \brief The Method executed in the thread, firing the actual notification.
     */
    void Run();

    /**
     * \brief Enum to identify the mode of the timer.
     */
    enum Mode
    {
        kOneshot,
        kPeriodic,
    };

    /**
     * \brief The function to call when the timer expires.
     */
    TimerHandler timer_handler_;

    /**
     * \brief The mode of the timer
     */
    Mode mode_;

    /**
     * \brief Flag to indicate to thread_ whether it should terminate.
     */
    std::atomic_bool exit_requested_;

    /**
     * \brief Flag to indicate whether the timer is currently active.
     */
    bool running_;

    /**
     * \brief The period of a periodic timer. The value is only valid if mode ==
     * kPeriodic.
     */
    Clock::duration period_;

    /**
     * \brief The next point in time when the timer will expire. This value is
     * only valid if running_ == true.
     */
    Clock::time_point next_expiry_point_;

    /**
     * \brief Mutex used by thread_ to wait on.
     *
     * Declared as mutable so access from const members is threadsafe.
     */
    mutable std::mutex mutex_;

    /**
     * \brief Condition Variable used by thread_ to wait on.
     */
    std::condition_variable condition_variable_;

    /**
     * \brief The Thread handling the timer notification.
     */
    std::thread thread_;

    /**
     * 
     */
    std::atomic<bool> flag_;
};

#endif /*  __COMMON_TIMER_H__ */