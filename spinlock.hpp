#ifndef DISTRIB_BASE_SPINLOCK_HEADER
#define DISTRIB_BASE_SPINLOCK_HEADER

namespace base {

class Spinlock
{
public:
  Spinlock();
  ~Spinlock();

  void lock();
  void unlock();

private:
  int locked_;

  // Non-copyable, non-assignable
  Spinlock(Spinlock&);
  Spinlock& operator=(Spinlock&);
}; 

}  // namespace base

#endif  //  DISTRIB_BASE_SPINLOCK_HEADER
