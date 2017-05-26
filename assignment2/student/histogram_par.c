#include "histogram.h"
#include <cstdlib>
#include <ctype.h>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <list>
#include "names.h"
#include <stdlib.h>

#define BLOCKSIZE  16384	// block size in byte



struct ThreadPool {
public:
    using Job = std::function<void(int* histogram)>;

    ThreadPool(unsigned int nofthreads ) :
                                nof_threads(nofthreads),
                                active(0),
                                finished(false),
                                threads(nofthreads)
    {
        histogram_array =  (int**)malloc(nof_threads* sizeof(int*));

        int i = 0;
        for (auto& t: threads)
        {
            histogram_array[i] = (int*)std::calloc( NNAMES, sizeof(int) );

            t = std::thread([=]() { process_jobs( &histogram_array[i][0] ); });
            i++;
        }
    }


    void getAll(int* histogram)
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


        for (unsigned int t = 0 ; t < nof_threads; t ++)
        {
            for(int k = 0; k < NNAMES; k++)
            {
                histogram[k] = histogram[k] + histogram_array[t][k];
            }
        }
    }


    ~ThreadPool()
    {

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
    std::mutex ende;
    std::condition_variable cv;
    std::list<Job> jobs;
    int** histogram_array;

    void process_jobs(int* histogram)
    {

        for(;;) {
            Job job;

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
            job(histogram);

            {
                std::unique_lock<std::mutex> lock(mutex);
                --active;
            }
        }
        /* if one thread finishes, all others have to finish as well */
        cv.notify_all();

    }
};

void countNames(size_t block_size, char* buffer, int* histogram)
{
    char current_word[20] = "";
    int c = 0;
    for (size_t i=0; buffer[i]!=TERMINATOR && i < block_size; i++) {

        if(isalpha(buffer[i]) && i%CHUNKSIZE!=0 ){
            current_word[c++] = buffer[i];
        } else {
            current_word[c] = '\0';
            int res = getNameIndex(current_word);
            if (res != -1)
                histogram[res]++;
            c = 0;
        }
    }
}


#include <iostream>


void get_histogram(char *buffer, int* histogram, int num_threads)
{

    ThreadPool tpool(num_threads);

    size_t block_size;


    while(buffer[0]!=TERMINATOR)
    {
        block_size = BLOCKSIZE;

        char *buffer_ = &buffer[0];

        tpool.submit([=](int* histogram_) { countNames( block_size, buffer_, histogram_); });

        buffer = &buffer[block_size];
    }

    tpool.getAll(histogram);

}
































