#include <iostream>
#include <fstream>
#include "vector_.h"
#include "bank.h"
#include "customer.h"
#include "utils.h"
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

sem_t sem;    //linux


void* runner(void* param) {           // thread runner
    Customer* c = (Customer*)param;

    sem_wait(&sem);
    Utils::print_locked("%s%d%s", "<<< Customer thread p#", c->get_id(), " started... >>>\n");
    sem_post(&sem);

    Bank* b = c->get_bank();
                          // TODO...
    int counter = 50;     // for debugging purposes, just stop after 50 iterations
                          //remove this when algorithm fully working

    while (!c->needs_met()) { //altered
        vector_<int> req = c->create_req();
        int idx = c->get_id();
        bool approved = b->req_approved(idx, req);

        if (approved) {
            sem_wait(&sem);
            Utils::print_locked("P%d requesting: %s    ............. request APPROVED!\n",
                          idx, (req.as_string()).c_str());
            c->alloc_req(req);
            b->withdraw_resources(req);
            b->show();
            sem_post(&sem);

            if (c->needs_met()) {
                sem_wait(&sem);
                b->deposit_resources(c->get_max());
                c->release_all_resources();
                b->show();
                sem_post(&sem);
            }
        }
        else {
            sem_wait(&sem);
            Utils::print_locked("P%d requesting: %s     --- Request DENIED\n", idx, (req.as_string()).c_str());
            sem_post(&sem);
        }
    }
    sem_wait(&sem);
    Utils::print_locked(">>>>>>>>>>>>>>> Customer thread p#%d shutting down... <<<<<<<<<<<<<<<<<\n\n", c->get_id());
    b->show();
    sem_post(&sem);

    pthread_exit(0);
}

void run_simulation(Bank* bank) {
    Utils::print_locked("\nBanker's algorithm simulation beginning...\n--------------------------------------------\n");
    bank->show();

    // start threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);    /* get the default attributes */

    vector_<Customer*> customers = bank->get_customers();
    for (Customer* c : customers) {
        pthread_create(c->get_threadid(), &attr, runner, c);
    }

    // join threads
    for (Customer* c : customers) {
        pthread_join(*c->get_threadid(), NULL);
    }
    Utils::print_locked("\nBanker's algorithm simulation completed...\n\n");
}

void process_line(char* buf, vector_<int>& values) {    // gets values from one line
  int i = 0;

  Utils::strstrip(buf);  // strip punctuation
  values.clear();

  char* p = strtok(buf, " ");   // split into tokens
  while (p != nullptr) {
    int val = atoi(p);
    values.push_back(val);      // convert to int, add to int array
    p = strtok(nullptr, " ");
    ++i;
  }
}

void process_filestream(std::ifstream& ifs, Bank*& bank) {    // extracts avail for Bank, customers' alloc and max
  char buf[BUFSIZ];
  vector_<int> res;  // resources

  bool finished = false;
  bool first_line = true;
  int idx = 0;
  while (!finished) {
    ifs.getline(buf, BUFSIZ, '\n');
    std::cout << "buf is: " << buf << "\n";

    if (strlen(buf) == 0) { finished = true;  break; }

    process_line(buf, res);
    if (first_line) {    // first line has bank's resources
      bank = new Bank(res);
      first_line = false;
    } else {
      vector_<int> alloc;
      vector_<int> max;
      size_t size = bank->get_avail().size();
      for (size_t i = 0; i < size; ++i) {
        alloc.add(res[i]);            // e.g., for size = 2,  0, 1
        max.add(res[i + size]);       // ditto,               2, 3
      }
      Customer* c = new Customer(idx++, alloc, max, bank);
      bank->add_customer(c);
    }
  }
  ifs.close();
}

 int process_files(int argc, const char* argv[]) {
        std::string filename;
        std::ifstream ifs;
        Bank* bank = nullptr;

    sem_init(&sem, 0, 1);   // OR use your version of sem_create

    while(argc--> 1) {
        filename = *++argv;
        ifs.open(filename);
        if (!ifs.is_open()) {
            std::cerr << "\n\nWarning, could not open file '" << filename << "'\n\n";
            exit(-1);
        }
        std::cout << "\n\n===================================================================================\n";
        std::cout << "//     PROCESSING FILENAME: " << filename << "\n";
        std::cout << "//================================================================================\n";

        if (bank != nullptr) {
            delete bank;
            bank = nullptr;
        }

        process_filestream(ifs, bank);
        run_simulation(bank);
        std::cout << "//===================================================================================\n";
        std::cout << "//==================== FINISHED PROCESSING FILENAME: " << filename << "\n";
        std::cout << "//===================================================================================\n\n\n";
    }
    
    sem_destroy(&sem);                  // OR use your version of sem_release
    return 0;
}

// int main(int argc, const char * argv[]) {
//     if (argc == 1) {
//         std::cerr << "Usage: ./bank_algo filename1 filename2 ...\n\n";
//         exit(-2);
//     }

//     return process_files(argc, argv);
// }


int main(int argc, const char * argv[]) {
    //prints out error message if the program is run without a filename
    if (argc != 2) {
        std::cerr << "Usage: ./bank_algo filename\n\n";
        exit(-2);
    }
    //get the file name from user and runs it
    std::string filename = argv[1];
    std::ifstream ifs(filename);
    //Error if the file can't open. Ex: invalid filename inputted
    if (!ifs.is_open()) {
      std::cerr << "\n\nWarning, could not open file '" << filename << "'\n\n";
      exit(-1);
    }

    // now we have the filestream open
    Bank* bank = nullptr;
    sem_init(&sem, 0, 1);

    process_filestream(ifs, bank);

    run_simulation(bank);

    sem_destroy(&sem);

    std::cout << "\n\t...done.  (all processes should show 0 resources left when finished)\n";
    std::cout << "\t\t... Test with all input files provided...\n";
    std::cout << "\t\t... data/bankers_tinier.txt, data/bankers_tiny.txt, ...\n\n";
    return 0;
}

//---------------the output-----------------------------//
// osc@ubuntu:~/final-src-osc10e/ch8/bankers_code$ g++ -pthread -std=c++17 -o bank_algo bank_algo.cpp
// osc@ubuntu:~/final-src-osc10e/ch8/bankers_code$ ./bank_algo bankers_tiny.txt
// buf is: 9, 9
// buf is: 2, 1,   3, 3
// buf is: 3, 3,   4, 3
// buf is: 1, 1,   2, 2
// buf is: 

// Banker's algorithm simulation beginning...
// --------------------------------------------

// +-----------------------------------------
// |   BANK   avail: [ 9  9]
// +-----------------------------------------
// | P#  0    2  1    3  3    1  2
// | P#  1    3  3    4  3    1  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// <<< Customer thread p#1 started... >>>
// P1 requesting:  2  0     --- Request DENIED
// P1 requesting:  2  0     --- Request DENIED
// P1 requesting:  1  0    ............. request APPROVED!

// +-----------------------------------------
// |   BANK   avail: [ 8  9]
// +-----------------------------------------
// | P#  0    2  1    3  3    1  2
// | P#  1    4  3    4  3    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// Customer p#1 has released all resources and is shutting down

// +-----------------------------------------
// |   BANK   avail: [12 12]
// +-----------------------------------------
// | P#  0    2  1    3  3    1  2
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// >>>>>>>>>>>>>>> Customer thread p#1 shutting down... <<<<<<<<<<<<<<<<<


// +-----------------------------------------
// |   BANK   avail: [12 12]
// +-----------------------------------------
// | P#  0    2  1    3  3    1  2
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// <<< Customer thread p#0 started... >>>
// P0 requesting:  2  3     --- Request DENIED
// P0 requesting:  2  2     --- Request DENIED
// P0 requesting:  1  1    ............. request APPROVED!

// +-----------------------------------------
// |   BANK   avail: [11 11]
// +-----------------------------------------
// | P#  0    3  2    3  3    0  1
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// P0 requesting:  0  2     --- Request DENIED
// P0 requesting:  0  3     --- Request DENIED
// P0 requesting:  0  2     --- Request DENIED
// P0 requesting:  0  3     --- Request DENIED
// P0 requesting:  0  2     --- Request DENIED
// P0 requesting:  0  3     --- Request DENIED
// P0 requesting:  0  2     --- Request DENIED
// P0 requesting:  0  1    ............. request APPROVED!

// +-----------------------------------------
// |   BANK   avail: [11 10]
// +-----------------------------------------
// | P#  0    3  3    3  3    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// Customer p#0 has released all resources and is shutting down

// +-----------------------------------------
// |   BANK   avail: [14 13]
// +-----------------------------------------
// | P#  0    0  0    0  0    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// >>>>>>>>>>>>>>> Customer thread p#0 shutting down... <<<<<<<<<<<<<<<<<


// +-----------------------------------------
// |   BANK   avail: [14 13]
// +-----------------------------------------
// | P#  0    0  0    0  0    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    1  1    2  2    1  1
// +----------------------------------------
// <<< Customer thread p#2 started... >>>
// P2 requesting:  1  2     --- Request DENIED
// P2 requesting:  2  3     --- Request DENIED
// P2 requesting:  3  1     --- Request DENIED
// P2 requesting:  1  3     --- Request DENIED
// P2 requesting:  3  3     --- Request DENIED
// P2 requesting:  2  2     --- Request DENIED
// P2 requesting:  2  3     --- Request DENIED
// P2 requesting:  1  1    ............. request APPROVED!

// +-----------------------------------------
// |   BANK   avail: [13 12]
// +-----------------------------------------
// | P#  0    0  0    0  0    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    2  2    2  2    0  0
// +----------------------------------------
// Customer p#2 has released all resources and is shutting down

// +-----------------------------------------
// |   BANK   avail: [15 14]
// +-----------------------------------------
// | P#  0    0  0    0  0    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    0  0    0  0    0  0
// +----------------------------------------
// >>>>>>>>>>>>>>> Customer thread p#2 shutting down... <<<<<<<<<<<<<<<<<


// +-----------------------------------------
// |   BANK   avail: [15 14]
// +-----------------------------------------
// | P#  0    0  0    0  0    0  0
// | P#  1    0  0    0  0    0  0
// | P#  2    0  0    0  0    0  0
// +----------------------------------------

// Banker's algorithm simulation completed...


//         ...done.  (all processes should show 0 resources left when finished)
//                 ... Test with all input files provided...
//                 ... data/bankers_tinier.txt, data/bankers_tiny.txt, ...

// osc@ubuntu:~/final-src-osc10e/ch8/bankers_code$ 