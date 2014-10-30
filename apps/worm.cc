// simple worm model based on message passing
// 

#include "worm.h"
#include "random.h"
#include "math.h"

// timer for sending probes
void ProbingTimer::expire(Event*) {
  t_->timeout();
}

// base class for worm: host is invulnerable by default
static class WormAppClass : public TclClass {
public:
	WormAppClass() : TclClass("Application/Worm") {}
	TclObject* create(int, const char*const*) {
		return (new WormApp());
	}
} class_app_worm;

// Initialize static variables
double WormApp::total_addr_ = pow(2, 32);
int WormApp::first_probe_ = 0;

WormApp::WormApp() : Application() {
  // get probing rate from configuration
  bind("ScanRate", &scan_rate_);

  // get probing port from configuration
  bind("ScanPort", &scan_port_);

  // get probing port from configuration
  bind("ScanPacketSize", &p_size_);
}

void WormApp::process_data(int nbytes, AppData*) {
  recv(nbytes);
}

void WormApp::recv(int) {
  if (!first_probe_) {
    first_probe_ = 1;
    printf("D FP %.2f\n", Scheduler::instance().clock());
  }

  //printf("D U %.2f %d\n",
  //       Scheduler::instance().clock(), my_addr_);
}

void WormApp::timeout() {
}

int WormApp::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();

  if (argc == 3) {
    if (strcmp(argv[1], "attach-agent") == 0) {
      agent_ = (Agent*) TclObject::lookup(argv[2]);
      if (agent_ == 0) {
	tcl.resultf("no such agent %s", argv[2]);
	return(TCL_ERROR);
      }
      agent_->attachApp(this);
      my_addr_ = agent_->addr();
      //printf("%d\n", my_addr_);
      return(TCL_OK);
    }
  }

  return(Application::command(argc, argv));
}

// Initialize stats (number of infected hosts) for DN
unsigned long DnhWormApp::infect_total_ = 0;
unsigned long DnhWormApp::addr_high_ = 0;
unsigned long DnhWormApp::addr_low_ = 0;
unsigned long DnhWormApp::default_gw_ = 0;
float DnhWormApp::local_p_ = 0;

// class to model vulnerable hosts in detailed network
static class DnhWormAppClass : public TclClass {
public:
	DnhWormAppClass() : TclClass("Application/Worm/Dnh") {}
	TclObject* create(int, const char*const*) {
		return (new DnhWormApp());
	}
} class_app_worm_dnh;

DnhWormApp::DnhWormApp() : WormApp() {
  infected_ = false;
  timer_ = NULL;
}

void DnhWormApp::recv(int) {
  if (infected_) {
    //printf("Node %d is infected already...\n", my_addr_);
  } else {
    if (!first_probe_) {
        first_probe_ = 1;
        printf("D FP %.2f\n", Scheduler::instance().clock());
    }

    printf("D C %.2f %lu %lu\n", 
    	   Scheduler::instance().clock(), infect_total_, my_addr_);
    
    // start to probe other hosts
    probe();
  }
}

void DnhWormApp::timeout() {
  timer_->resched(p_inv_);
  send_probe();
}

void DnhWormApp::probe() {
  infected_ = true;
  infect_total_++;


  if (scan_rate_) {
    p_inv_ = 1.0 / scan_rate_;

    timer_ = new ProbingTimer((WormApp *)this);
    timer_->sched(p_inv_);
  }
}

void DnhWormApp::send_probe() {
  double range_low, range_high;
  unsigned long d_addr;
  ns_addr_t dst;

  // do not probe myself
  d_addr = my_addr_;
  
  if (Random::uniform(0.0, 1.0) < local_p_) {
    range_low = addr_low_;
    range_high = addr_high_;
  } else {
    range_low = 0;
    range_high = total_addr_;
  }
  
  while (d_addr == my_addr_)
    d_addr = static_cast<unsigned long>(Random::uniform(range_low, range_high));

  // probe within my AS
  if (addr_low_ <= d_addr && d_addr <= addr_high_) {
    //printf("D PD %.2f %d %d\n", 
	//   Scheduler::instance().clock(), my_addr_, d_addr);
  } else {
    //printf("Node %d is probing node %d, within AN, send to node %d\n", 
    //	   my_addr_, d_addr, default_gw_);
  }

  dst.addr_ = d_addr;
  dst.port_ = scan_port_;
  agent_->sendto((int)p_size_, (const char *)NULL, dst);
}

int DnhWormApp::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "gw") == 0) {
      default_gw_ = atol(argv[2]);
      return(TCL_OK);
    }
    if (strcmp(argv[1], "local-p") == 0) {
        local_p_ = atof(argv[2]);
        return(TCL_OK);
    }   
  }
  if (argc == 4) {
    if (strcmp(argv[1], "addr-range") == 0) {
      addr_low_ = atol(argv[2]);
      addr_high_ = atol(argv[3]);
      //printf("DN low: %d, high: %d\n", addr_low_, addr_high_);
      return(TCL_OK);
    }
  }

  return(WormApp::command(argc, argv));
}

// class to model vulnerable hosts in detailed network
static class AnWormAppClass : public TclClass {
public:
	AnWormAppClass() : TclClass("Application/Worm/An") {}
	TclObject* create(int, const char*const*) {
		return (new AnWormApp());
	}
} class_app_worm_an;

AnWormApp::AnWormApp() : WormApp() {
  // using 1 second as the unit of time step
  //time_step_ = 1;
  timer_ = NULL;

  addr_low_ = addr_high_ = my_addr_;
  s_ = i_ = 0;
  v_percentage_ = 0;
  beta_ = gamma_ = 0;
  n_ = r_ = 1;
  
  probe_in = probe_out = probe_recv = 0;

  // get time step from configuration
  bind("TimeStep", &time_step_);
}

void AnWormApp::start() {
  // initial value of i and s
  i_ = 1;
  s_ -= 1;

  timer_ = new ProbingTimer((WormApp *)this);
  timer_->sched((double)time_step_);

  //printf("start\n");
}

void AnWormApp::recv(int) {
  probe_recv++;
  
  //printf("AN (%d) received probes from outside...%f \n", my_addr_, probe_recv);
}

void AnWormApp::timeout() {
  //printf("timeout\n");
  timer_->resched((double)time_step_);
  update();
}

void AnWormApp::update() {
  // schedule next timeout
  timer_->resched(time_step_);

  probe_out = scan_rate_ * i_ * (dn_high_ - dn_low_ + 1)  * time_step_ / total_addr_;
  // not every probe received has effect
  probe_in = probe_recv * s_ / n_;
  probe_recv = 0;

  // update states in abstract networks
  // update r (recovered/removed)
  r_ = r_ + gamma_ * i_;
  if (r_ < 0)
    r_ = 0;
  if (r_ > n_)
    r_ = n_;

  // update i (infected)
  // contains four parts:
  // 1. i of last time period, 2. increase due to internal probing
  // 3. decrease due to internal recovery/removal,
  // 4. increase due to external probing

  // should use n_ or s_max_???
  //i_ = i_ + beta_ * i_ * (s_ / n_) * time_step_ - gamma_ * i_ + probe_in;
  i_ = i_ + beta_ * i_ * (s_ / s_max_) * time_step_ - gamma_ * i_ + probe_in;
  if (i_ < 0)
    i_ = 0;
  if (i_ > n_ - r_)
    i_ = n_ - r_;
 
  // update s (susceptible)
  // use n = r + i + s
  s_ = n_ - r_ - i_;

  printf("A %.2f %d %d %d %d %d\n",
	 Scheduler::instance().clock(),
	 (int)s_, (int)i_, (int)r_, (int)probe_in, (int)probe_out);

  // probe outside networks
  // should not be cumulated!!!
  //probe_out = 2;
  if (probe_out > 1) { 
    //printf("ANS %.2f %d %d %d %d %d\n", 
    //        Scheduler::instance().clock(), 
    //        (int)s_, (int)i_, (int)r_, (int)probe_in, (int)probe_out);
    probe((int)(probe_out + 0.5));
  }
}

void AnWormApp::probe(int times) {
  // send out probes in a batch
  int i;
  unsigned long d_addr;
  ns_addr_t dst;

  i = 0;
  while (i < times) {
    d_addr = dn_low_ + (int)Random::uniform(dn_high_ - dn_low_);

    // do not send to myself or AS
    if (dn_low_ < d_addr && d_addr < dn_high_) {
      // probe outside
      //printf("AN is probing node %d, outside AN\n", d_addr);

      dst.addr_ = d_addr;
      dst.port_ = scan_port_;
      agent_->sendto((int)p_size_, (const char *)NULL, dst);

      i++;
    }
  }
}

int AnWormApp::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "v_percent") == 0) {
      if (n_ == 0) {
	printf("space range is not specificed!\n");
	return(TCL_ERROR);
      } else {
	v_percentage_ = atof(argv[2]);
	
	s_ = (int)(n_ * v_percentage_);
	if (s_ < 1)
	  s_ = 1;
	s_max_ = s_;
	r_ = n_ - s_;

	// use the equation in Moore's Internet Quarantine paper:
	// beta = scan_rate * total_vulnerable / 2^32
	beta_ = scan_rate_ * s_max_ / total_addr_;
	//printf("inferred beta from scan rate: %f, %f, %d, %f\n", 
	//	beta_, scan_rate_, (int)s_max_, total_addr_);
	return(TCL_OK);
      }
    }
    if (strcmp(argv[1], "beta") == 0) {
      beta_ = atof(argv[2]);
      //printf("beta: %f\n", beta_);

      return(TCL_OK);
    }
    if (strcmp(argv[1], "gamma") == 0) {
      gamma_ = atof(argv[2]);
      //printf("gamma: %f\n", gamma_);

      return(TCL_OK);
    }
  }
  if (argc == 4) {
    if (strcmp(argv[1], "addr-range") == 0) {
      addr_low_ = atol(argv[2]);
      addr_high_ = atol(argv[3]);

      // initialize SIR model states
      n_ = addr_high_ - addr_low_ + 1;
      //printf("AN low: %d, high: %d, n: %f\n", addr_low_, addr_high_, n_);
      return(TCL_OK);
    }
    if (strcmp(argv[1], "dn-range") == 0) {
      dn_low_ = atoi(argv[2]);
      dn_high_ = atoi(argv[3]);
      //printf("AN-DN low: %d, high: %d\n", dn_low_, dn_high_);

      return(TCL_OK);
    }
  }

  return(WormApp::command(argc, argv));
}
