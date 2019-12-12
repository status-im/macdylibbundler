#include <algorithm>
#include <thread>
#include <functional>
#include <vector>
// #include <pthread.h>

// typedef void *(*THREADFUNCPTR)(void *);

/**
 * @param nb_elements size of your for loop
 * @param functor(start, end) : your function processing a sub chunk of the for loop.
 * "start" is the first index to process (included) until the index "end" (excluded)
 * @code
 *     for(int i = start; i < end; ++i)
 *         computation(i);
 * @endcode
 * @param use_threads : enable / disable threads.
 *
 */
static void parallel_for(unsigned nb_elements,
                         std::function<void (int start, int end)> functor,
                         bool use_threads = true)
{
    // unsigned nb_threads_hint = std::thread::hardware_concurrency();
    // unsigned nb_threads = nb_threads_hint == 0 ? 8 : (nb_threads_hint);

    unsigned nb_threads = 8;

    unsigned batch_size = nb_elements / nb_threads;
    unsigned batch_remainder = nb_elements % nb_threads;

    // std::vector<std::thread> my_threads(nb_threads);
    std::vector<std::thread> my_threads(nb_threads+1);

    // pthread_t threads[nb_threads];
    // pthread_attr_t attr;
    int rc;
    int i;
    void *status;

    // Initialize and set thread joinable
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (use_threads) {
        // Multithread execution
        for(unsigned i = 0; i < nb_threads; ++i) {
        // for (i = 0; i < nb_threads; ++i) {
            int start = i * batch_size;
            my_threads[i] = std::thread(functor, start, start+batch_size);
            // rc = pthread_create(&threads[i], &attr, (THREADFUNCPTR)&functor, (void *)i);
            // if (rc) {
            //     std::cout << "Error:unable to create thread," << rc << std::endl;
            //     exit(-1);
            // }
            // std::cout << "pthread created" << std::endl;
        }

        // free attribute and wait for the other threads
        // pthread_attr_destroy(&attr);
        // for (i = 0; i < nb_threads; ++i) {
        //     rc = pthread_join(threads[i], &status);
        //     if (rc) {
        //         std::cout << "Error:unable to join," << rc << std::endl;
        //         exit(-1);
        //     }
        //     std::cout << "Main: completed thread id :" << i ;
        //     std::cout << "  exiting with status :" << status << std::endl;
        // }

        // std::cout << "Main: program exiting." << std::endl;
        // pthread_exit(NULL);
    }
    else {
        // Single thread execution (for easy debugging)
        for (unsigned i = 0; i < nb_threads; ++i){
            int start = i * batch_size;
            functor(start, start+batch_size);
        }
    }

    // Deform the elements left
    int start = nb_threads * batch_size;
    // functor(start, start+batch_remainder);
    my_threads[nb_threads] = std::thread(functor, start, start+batch_remainder);

    // Wait for the other thread to finish their task
    if(use_threads)
        std::for_each(my_threads.begin(), my_threads.end(), std::mem_fn(&std::thread::join));
}
