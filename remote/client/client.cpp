#include <qsim.h>
#include <iostream>
#include <vector>

#include "qsim-client.h"

using std::cout;

struct CbTester {
  CbTester(): running(true) {}

  int atomic(int cpu) {
    return 0;
  }

  void inst(int cpu, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *b,
            enum inst_type t)
  {
    static unsigned count = 0;
    if (count++ == 10000) count = 0; else return;
    cout << "Instruction, CPU " << cpu << " va=0x" << std::hex << va << ": ";
    for (unsigned i = 0; i < l; i++) 
      cout << std::hex << std::setw(0) << std::setfill('0') << (b[i]&0xff) 
           << ' ';
    cout << '\n';
  }

  void mem(int cpu, uint64_t va, uint64_t pa, uint8_t s, int t) {
    static unsigned count = 0; 
    if (count++ == 10000) count = 0; else return;
    cout << "Memory op, CPU " << cpu << " va=0x" << std::hex << va << " size="
         << (s&0xff) << ' ' << (t?'W':'R') << '\n';
  }

  void app_end(int cpu) { running = false; }

  bool running;
} cbtest;

int main(int argc, char **argv) {
  int cpu(-1);

  if (argc != 3 && argc != 4) {
    cout << "Usage: " << argv[0] << " <server> <port> [core #]\n";
    return 1;
  }

  Qsim::Client *qc = new Qsim::Client(client_socket(argv[1], argv[2]));
  if (argc == 4) cpu = atol(argv[3]);

  qc->set_atomic_cb (&cbtest, &CbTester::atomic );
  qc->set_inst_cb   (&cbtest, &CbTester::inst   );
  qc->set_mem_cb    (&cbtest, &CbTester::mem    );
  //qc->set_app_end_cb(&cbtest, &CbTester::app_end);

  cout << "Server has " << qc->get_n() << " CPUs.\n";
  for (unsigned i = 0; i < qc->get_n(); i++) {
    cout << "  CPU " << i << " is " << (qc->booted(i)?"booted.\n":"offline.\n");
    cout << "  Running TID " << qc->get_tid(i) << '\n';
    cout << "  " << ((qc->get_prot(i))?"Kernel mode.\n":"User mode.\n");
    cout << "  " << ((qc->get_mode(i))?"Protected mode.\n":"Real mode.\n");
  }

  for (unsigned i = 0; i < 100; ++i) { // Run for 100M instructions
    if (cpu == -1)
      for (unsigned j = 0; j < qc->get_n(); j++) qc->run(j, 1000000);
    else
      qc->run(cpu, 1000000);
    if (cpu == 0) qc->timer_interrupt();
  }

  delete qc;

  return 0;
}
