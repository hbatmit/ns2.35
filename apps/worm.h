// worm propagation model

#ifndef ns_worm_h
#define ns_worm_h

#include "timer-handler.h"
#include "app.h"

class WormApp;

// timer to control worm scan rate, etc
class ProbingTimer : public TimerHandler {
 public:
  ProbingTimer(WormApp* t) : TimerHandler(), t_(t) {};
  inline virtual void expire(Event*);
 protected:
  WormApp* t_;
};

// base class, by default hosts are NOT vulnerable
class WormApp : public  Application {
 public:
  WormApp();

  // timer handler
  virtual void timeout();
  // agent call back function
  void process_data(int, AppData*);

  virtual int command(int argc, const char*const* argv);

 protected:
  // need to define recv and timeout 
  virtual void recv(int nbytes);

  // id of the node attached
  unsigned long  my_addr_;

  // the toal Internet address space
  static double total_addr_;
  // flag to record first probe
  static int first_probe_;

  // configs for worm probing behavior
  double scan_rate_;
  int scan_port_;
  int p_size_;
};

// model invulnerable hosts in detailed networks
class DnhWormApp : public WormApp {
 public:
  DnhWormApp();

  // timer handler
  void timeout();

  int command(int argc, const char*const* argv);

 protected:
  void recv(int nbytes);

  void send_probe();
  void probe();

  bool infected_;
  static unsigned long infect_total_;

  ProbingTimer *timer_;

  // control the rate of probing
  double p_inv_;

  // the address space of my networks
  static unsigned long addr_low_, addr_high_;
  
  // the probability to scan local hosts
  static float local_p_;
  
  // the access point to other networks like AN
  static unsigned long default_gw_;
};

// model a network with SIR model
class AnWormApp : public WormApp {
 public:
  AnWormApp();

  // timer handler
  void timeout();

  int command(int argc, const char*const* argv);

 protected:
  void start();
  void update();
 
  void recv(int nbytes);

  void probe(int);

  ProbingTimer *timer_;

  int time_step_;

  // the address space of my networks
  unsigned long addr_low_, addr_high_;
  unsigned long dn_low_, dn_high_;

  // SIR model states:
  double s_, i_, r_, s_max_, n_;
  double v_percentage_;
  double beta_, gamma_;

  // interaction with DN
  double probe_in, probe_recv;
  double probe_out;
};


#endif
