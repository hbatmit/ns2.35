#include<stdio.h>
#define hop_limit 50

struct flow_list{
double time;
double bw;
struct flow_list *next;
};

struct flow_event{
  double delay;
  double deltaCBR;
  double NetCBR;
};

struct flow_list *create_new_flow_event(double time, double bw);

void flow_list_insert(double time, double bw, struct flow_list **root);

void dump_flow_list_to_file(FILE* file, struct flow_list *root);

int get_no_events(struct flow_list *root);

struct flow_event* get_all_events(struct flow_list *root);

