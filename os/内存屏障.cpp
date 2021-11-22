#include <iostream>
#include <pthread.h>
#include <stdlib.h>

using namespace std;

typedef unsigned long long uul;
static uul global = 0;

void *func(void *argv) {
    uul loop = *((uul *)argv);

    for (uul i = 0; i < loop; ++i) {
        global += 1;
    }

    return 0;
}

int main() {
    pthread_t p1, p2;
    uul loop = 10000000;

    pthread_create(&p1, nullptr, func, &loop);
    pthread_create(&p2, nullptr, func, &loop);
    pthread_join(p1, nullptr);
    pthread_join(p2, nullptr);
    cout << "global: " << global << endl;

    return 0;
}


#include <atomic>
#include <iostream>
#include <thread>

using namespace std;

atomic<int> a{0};
atomic<int> b{0};

int valueset(int) {
    a.store(1, memory_order_relaxed);
    //b.store(2, memory_order_relaxed);
    b.store(2, memory_order_release);

    return 0;
}

int observer(int) {
  while (b.load(memory_order_acquire) != 2) // 自旋等待
    ;
  cout << "observer " << a.load(memory_order_relaxed) << endl;
  return 0;
}

int main() {
    thread t1(valueset, 0);
  thread t2(observer, 0);

  t1.join();
  t2.join();
  cout << "Got (" << a << ", " << b << ")" << endl;
    return 0;
}