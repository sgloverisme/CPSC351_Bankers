//
//  bank.h
//  bankers
//
//  Created by William McCarthy on 21 Mar 2022.
//

#ifndef __bank_h__
#define __bank_h__

#include <iostream>
#include <mutex>
#include <sstream>
#include "customer.h"
#include "vector_.h"


class Bank {
public:
  Bank() = default;
  Bank(const vector_<int>& available) : avail(available), customers() { }
  
  vector_<int> get_avail() const { return avail; }
  bool is_avail(const vector_<int>& req) const { return req < avail; }
  
       // TODO --- correctly implement the Banker's algorithm for 
       //    is_safe() and req_approved()
 
  bool is_safe(int id, const vector_<int>& req) { 
    /*  alloc , max , and need
      taking in the customer ID and a vector/list of requests(processor) 
      goal: is it safe (no deadlock). must calculate the matrices
      go through the matrix by row and then by column then move to next row 
    
    */
    Customer* c = customers[id]; //making pointer
    bool clear =false; // a flag to move on to next row or not
    for (int i=0; i <customers.size(); i++){ //row 
      vector_<int> extra = customers[i]->get_max(); // the difference 
      for(int j=0; j<extra.size(); j++){ // going through column
        if (extra[j] <= avail[j] - req[j]) //if enough, deem as true
          clear = true;
          else{
            return clear; // not enough, deem false
          }
      } //end of 2nd for loop (colmun)
    } //endo 1st for loop (row)
    return this;
    //return true; 
  } //end of is_safe()   
  bool req_approved(int id, const vector_<int>& req) { 
    Customer* c = customers[id]; //customer pointer
    if(c->too_much(req)){
      return false;
    }else{
      return is_safe(id, req); //added
    }
    return !c->too_much(req);  
  }
  
  void add_customer(Customer* c) { customers.push_back(c); }
  
  void withdraw_resources(const vector_<int>& req) {
    if (!is_avail(req)) {
      Utils::print_locked("WARNING: req: %s is not available for withdrawing\n", (req.as_string().c_str()));
      return;
    }
    if (is_avail(req)) { avail -= req; }
  }
  void deposit_resources(const vector_<int>& req) { avail += req; }


  vector_<Customer*> get_customers() const { return customers; }
  
  void show() const { 
    std::stringstream ss;
    ss << "\n+-----------------------------------------\n"
          "|   BANK   avail: [" << avail.as_string().c_str() << "]\n"
          "+-----------------------------------------\n";
    for (Customer* c : customers) {
      ss << "| P# " <<  std::setw(2) << c->get_idx() << "   "
         <<  (c->get_alloc().as_string().c_str()) << "   "
         << (c->get_max().as_string().c_str())    << "   "
         << (c->get_need().as_string().c_str())
         << "\n";
    }
    std::string s = ss.str();

    Utils::print_locked("%s"
                        "+----------------------------------------\n", (s.c_str()));
  }
  
  friend std::ostream& operator<<(std::ostream& os, const Bank& bank) {
    bank.show();
    return os;
  }

private:
  vector_<int> avail;
  vector_<Customer*> customers;
};

#endif /* __bank_h__ */
