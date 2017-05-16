#include "histogram.h"
#include <cstdlib>
#include <ctype.h>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <list>
#include "names.h"

#define BLOCKSIZE 8192 	// block size in byte


struct ThreadPool {
public:
    using Job = std::function<void()>;

    ThreadPool(unsigned int nofthreads, int* histogram_in, ) :
                                nof_threads(nofthreads),
                                active(0),
                                finished(false),
                                threads(nofthreads)
    {
        for (auto& t: threads)
        {
            t = std::thread([=]() { process_jobs(t); });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            finished = true;
        }

        cv.notify_all();
        for (auto& t: threads)
        {
                t.join();
        }
    }

    void submit(Job job)
    {
        std::unique_lock<std::mutex> lock(mutex);
        jobs.push_back(std::move(job));
        cv.notify_one();
    }

private:
    unsigned int nof_threads;
    unsigned int active;
    bool finished;
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::condition_variable cv;
    std::list<Job> jobs;

    void process_jobs(int i)
    {
        for(;;) {
            Job job;
            int histogram[NNAMES];
            /* fetch job */
            {
                std::unique_lock<std::mutex> lock(mutex);
                while (jobs.empty() && (active > 0 || !finished))
                {
                    cv.wait(lock);
                }

                if (jobs.empty() && active == 0 && finished)
                    break;
                job = std::move(jobs.front());
                jobs.pop_front();
                ++active;
            }

            /* execute job */
            job();

            {
                std::unique_lock<std::mutex> lock(mutex);
                --active;
            }
        }
        /* if one thread finishes, all others have to finish as well */
        cv.notify_all();
    }
};


typedef struct{
    int* histogram; //[NNAMES];
    char* blocks;   // start pointer
    int block_size;

}threadData_t;


void countNames(int block_size, char* buffer,  int* histogram)
{
    char current_word[20] = "";
    int c = 0;

    for (int i=0; buffer[i]!=TERMINATOR && i < block_size; i++)
    {
        if(isalpha(buffer[i]) && i%CHUNKSIZE!=0 )
        {
            current_word[c++] = buffer[i];
        }else{
            current_word[c] = '\0';
            int res = getNameIndex(current_word);
            if (res != -1)
                histogram[res]++;
            c = 0;
        }
    }
}



void get_histogram(char *buffer, int* histogram, int num_threads)
{

    ThreadPool tpool(num_threads, histogram);

    int block_size;

    for (int i=0; buffer[i]!=TERMINATOR &&; i = i + block_size)
    {
        block_size = BLOCKSIZE;
        while(buffer[block_size] != " ")
        {
            block_size++;
        }

       tpool.submit(  [=]() -> Result { return countNames( block_size, buffer[i]); });
    }

    tpool.~ThreadPool();

}
































