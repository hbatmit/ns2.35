#include <algorithm>
#include "link/ensemble-rate-generator.h"

static class EnsembleRateGeneratorClass : public TclClass {
 public :
  EnsembleRateGeneratorClass() : TclClass("EnsembleRateGenerator") {}
  TclObject* create(int argc, const char*const* argv) {
    return (new EnsembleRateGenerator(std::string(argv[4]), atof(argv[5])));
  }
} class_fcfs;

EnsembleRateGenerator::EnsembleRateGenerator(std::string t_trace_file, double simulation_time)
    : link_rate_changes_(std::queue<LinkRateEvent>()),
      initial_rate_map_(std::map<uint32_t,double>()),
      trace_file_(t_trace_file),
      cap_in_bits_(std::map<uint32_t,double>()),
      last_change_(std::map<uint32_t,double>()),
      last_rate_(std::map<uint32_t,double>()),
      cap_in_bps_(std::map<uint32_t,double>()),
      num_users_(read_link_rate_trace(simulation_time)),
      user_links_(std::vector<LinkDelay*>(num_users_)),
      next_event_(LinkRateEvent()) {
  /* Finalize capacities */
  fprintf(stderr, "Just before finalizing caps \n");
  finalize_caps(simulation_time);

  fprintf(stderr, "EnsembleRateGenerator, num_users_ %u, trace_file_ %s, simulation_time %f \n",
          num_users_, trace_file_.c_str(), simulation_time);
}

int EnsembleRateGenerator::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if ( strcmp(argv[1], "get_users_" ) == 0 ) {
      Tcl::instance().resultf("%u",num_users_);
      return TCL_OK;
    }
    if ( strcmp(argv[1], "activate-rate-generator" ) == 0 ) {
      init();
      return TCL_OK;
    }
  }
  if (argc == 3) {
    if ( strcmp(argv[1], "get_initial_rate" ) == 0 ) {
      uint32_t user_id = atoi(argv[2]);
      Tcl::instance().resultf("%f", initial_rate_map_.at(user_id));
      return TCL_OK;
    }
    if ( strcmp(argv[1], "get_capacity" ) == 0 ) {
      uint32_t user_id = atoi(argv[2]);
      Tcl::instance().resultf("%f", cap_in_bps_.at(user_id));
      return TCL_OK;
    }
  }

  if(argc == 4) {
    if(!strcmp(argv[1],"attach-link")) {
      LinkDelay* link = (LinkDelay*) TclObject::lookup(argv[2]);
      uint32_t user_id = atoi(argv[3]);
      assert(user_id<user_links_.size());
      user_links_.at( user_id ) = link;
      return TCL_OK;
    }
  }
  return TclObject::command( argc, argv );
}

void EnsembleRateGenerator::expire(Event* e) {
  assert(Scheduler::instance().clock() >= next_event_.timestamp);
  assert(user_links_.at(next_event_.user_id)!=nullptr);
  user_links_.at(next_event_.user_id)->set_bandwidth(next_event_.link_rate);
  schedule_next_event();
}

uint32_t EnsembleRateGenerator::read_link_rate_trace(double simulation_time) {
  assert(trace_file_!="");
  FILE* f = fopen(trace_file_.c_str(), "r");
  if (f == NULL) {
    perror("fopen");
    exit(1);
  }
  assert(link_rate_changes_.empty());
  std::vector<uint32_t> unique_user_list;
  std::vector<LinkRateEvent> link_rate_change_vector;

  /* Populate event vector from file */
  while(1) {
    double ts, rate;
    uint32_t user_id;
    int num_matched = fscanf(f, "%lf %u %lf\n", &ts, &user_id, &rate);
    if (num_matched != 3) {
      break;
    }
    if (ts > simulation_time) {
      continue;
    }
    link_rate_change_vector.push_back(LinkRateEvent(ts, user_id, rate));
    if (std::find(unique_user_list.begin(), unique_user_list.end(), user_id) == unique_user_list.end()) {
      /* First link rate entry for this user */
      assert(ts == 0.0);
      unique_user_list.push_back(user_id);
      fprintf(stderr, "Populating initial rate %f for user %u \n", rate, user_id);
      initial_rate_map_[user_id] = rate;

      /* Initialize capacity calculation */
      cap_in_bits_[user_id] = 0.0;
      last_rate_[user_id] = rate;
      last_change_[user_id] = ts;
    } else {
      /* Update capacity */
      assert(ts > last_change_.at(user_id));
      cap_in_bits_.at(user_id) += (ts - last_change_.at(user_id)) * last_rate_.at(user_id);
      last_rate_.at(user_id) = rate;
      last_change_.at(user_id) = ts;
    }
  }

  /* Sort link_rate_change_vector */
  std::sort(link_rate_change_vector.begin(), link_rate_change_vector.end(),
            [&] (const LinkRateEvent &e1, const LinkRateEvent &e2)
            { return e1.timestamp < e2.timestamp; });

  /* Write into queue */
  for (uint32_t i=0; i < link_rate_change_vector.size(); i++) {
    if (!link_rate_changes_.empty()) {
      assert(link_rate_change_vector.at(i).timestamp >= link_rate_changes_.back().timestamp);
    }
    link_rate_changes_.push(link_rate_change_vector.at(i));
  }

  fclose(f);
  assert(!unique_user_list.empty());
  return unique_user_list.size();
}

void EnsembleRateGenerator::schedule_next_event() {
  if (link_rate_changes_.empty()) {
    force_cancel();
    return;
  }
  next_event_ = link_rate_changes_.front();
  link_rate_changes_.pop();
  assert(next_event_.timestamp >= Scheduler::instance().clock());
  auto time_to_next_event = next_event_.timestamp - Scheduler::instance().clock();
  resched(time_to_next_event);
}

void EnsembleRateGenerator::finalize_caps(double simulation_time) {
  /* Finalize capacity calculation */
  for (auto &x : cap_in_bits_) {
    cap_in_bits_.at(x.first) += (simulation_time - last_change_.at(x.first)) * last_rate_.at(x.first);
  }

  /* Normalize by dividing by simtime, to get throughput */
  for (auto &x : cap_in_bits_) {
    cap_in_bps_[x.first] = cap_in_bits_.at(x.first)/simulation_time;
  }
}

void EnsembleRateGenerator::init() {
  schedule_next_event();
}
