#include <getopt.h>
#include <stdlib.h>

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "BoundedBuffer.h"

#define CAP 10
#define SIZE 15
#define NUM 1

using namespace std;

void make_word (char* buf, int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = (rand() % 256) - 128;
    }
}

mutex mtx;
void add_word (vector<char*>* words, char* wrd) {
    mtx.lock();
    words->push_back(wrd);
    mtx.unlock();
}

void push_thread_function (char* wrd, int size, BoundedBuffer* bb) {
    bb->push(wrd, size);
}

void pop_thread_function (int size, BoundedBuffer* bb, vector<char*>* words) {
    char* wrd = new char[size];
    int read = bb->pop(wrd, size);
    if (read != size) {
        return;
    }

    mtx.lock();
    for (vector<char*>::iterator iter = words->begin(); iter != words->end(); ++iter) {
        char* cur = *iter;
        bool equal = true;
        for (int i = 0; i < size; i++) {
            if (wrd[i] != cur[i]) {
                equal = false;
                break;
            }
        }
        if (equal) {
            words->erase(iter);
            delete[] cur;
            break;
        }
    }
    mtx.unlock();

    delete[] wrd;
}

int main (int argc, char** argv) {
    int bbcap = CAP;
    int wsize = SIZE;
    int nthrd = NUM;

    // CLI option to change BoundedBuffer capacity, word size, and number of threads
    int opt;
    static struct option long_options[] = {
        {"bbcap", required_argument, nullptr, 'b'},
        {"wsize", required_argument, nullptr, 's'},
        {"nthrd", required_argument, nullptr, 'n'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "b:s:n:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'b':
                bbcap = atoi(optarg);
                break;
            case 's':
                wsize = atoi(optarg);
                break;
            case 'n':
                nthrd = atoi(optarg);
                break;
        }
    }

    // initialize overhead
    srand(time(nullptr));

    BoundedBuffer bb(bbcap);

    thread** push_thrds = new thread*[nthrd];
    thread** pop_thrds = new thread*[nthrd];
    for (int i = 0; i < nthrd; i++) {
        push_thrds[i] = nullptr;
        pop_thrds[i] = nullptr;
    }

    vector<char*> words;
    int count = 0;

    // process commands to test
    string type;
    int thrd;
    while (cin >> type >> thrd) {
        if (type == "push") {
            char* wrd = new char[wsize];
            make_word(wrd, wsize);
            add_word(&words, wrd);
            push_thrds[thrd] = new thread(push_thread_function, wrd, wsize, &bb);
            count++;
        }
        else if (type == "pop") {
            pop_thrds[thrd] = new thread(pop_thread_function, wsize, &bb, &words);
            count--;
        }
    }

    // joining all threads
    for (int i = 0; i < nthrd; i++) {
        if (push_thrds[i] != nullptr) {
            push_thrds[i]->join();
        }
        delete push_thrds[i];
        
        if (pop_thrds[i] != nullptr) {
            pop_thrds[i]->join();
        }
        delete pop_thrds[i];
    }

    // determining exit status
    int status = 0;
    if ((size_t) count != words.size()) {
        status = 1;
    }

    // clean-up head allocated memory
    for (auto wrd : words) {
        delete[] wrd;
    }
    delete[] push_thrds;
    delete[] pop_thrds;

    return status;
}
